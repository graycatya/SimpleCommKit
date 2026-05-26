"""SimpleCommKitAiSerialPort MCP Server - Expose serial port operations as MCP tools for AI agents."""

from fastmcp import FastMCP
import argparse
from typing import Dict, List, Optional

from SimpleCommKitAiSerialPort import (
    SerialPort,
    SerialPortInfo,
    BaudRate,
    Parity,
    DataBits,
    StopBits,
    FlowControl,
    get_error_description,
)


class PortState:
    """Holds global serial port state shared across MCP tools."""

    def __init__(self) -> None:
        self._ports: Dict[str, SerialPort] = {}
        self._read_buffer: Dict[str, List[Dict]] = {}
        self._initialized: bool = False

    def init(self) -> None:
        """Initialize serial port subsystem. Must be called before any tools are used."""
        self._initialized = True

    def ensure_initialized(self) -> None:
        """Raise if PortState has not been initialized yet."""
        if not self._initialized:
            raise RuntimeError(
                "Serial port service is still initializing. Please wait and try again."
            )

    @staticmethod
    def get_available_ports() -> List[Dict]:
        """Enumerate available serial ports."""
        try:
            ports = SerialPort.get_available_ports()
            result = []
            for p in ports:
                try:
                    desc = p.description
                except UnicodeDecodeError:
                    # On Windows, descriptions may be system-ANSI encoded (e.g. GBK).
                    # If the C++ layer hasn't converted to UTF-8 yet, fall back.
                    desc = p.description.encode("latin-1").decode("gbk", errors="replace")
                try:
                    hwid = p.hardware_id
                except UnicodeDecodeError:
                    hwid = p.hardware_id.encode("latin-1").decode("gbk", errors="replace")
                result.append({
                    "port_name": p.port_name,
                    "description": desc,
                    "hardware_id": hwid,
                })
            return result
        except Exception as e:
            return [{"error": str(e)}]

    def get_or_create(self, port_name: str) -> SerialPort:
        """Return existing SerialPort or create a new one."""
        if port_name not in self._ports:
            sp = SerialPort()
            sp.set_port_name(port_name)
            sp.set_callback_error(
                lambda code: print(
                    f"[SimpleCommKitAiSerialPort] Error {code}: "
                    f"{get_error_description(code)}"
                )
            )
            self._ports[port_name] = sp
            self._read_buffer.setdefault(port_name, [])
        return self._ports[port_name]

    def remove(self, port_name: str) -> None:
        self._ports.pop(port_name, None)
        self._read_buffer.pop(port_name, None)

    def ensure_port(self, port_name: str) -> SerialPort:
        if port_name not in self._ports:
            raise RuntimeError(
                f"Port '{port_name}' not found. Use get_available_ports to list ports, "
                f"then open_port to open one."
            )
        return self._ports[port_name]


port_state = PortState()
mcp = FastMCP(name="SimpleCommKitAiSerialPort MCP Server")


@mcp.tool(
    name="get_available_ports",
    description="List all available serial ports on the host system.",
    tags={"serial", "port", "read"},
    annotations={
        "title": "Get Available Ports",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "discovery"},
)
def get_available_ports() -> List[Dict[str, str]]:
    """List all available serial ports."""
    port_state.ensure_initialized()
    return port_state.get_available_ports()


@mcp.tool(
    name="open_port",
    description="Open a serial port with the given configuration. "
                "Use get_available_ports first to discover available port names.",
    tags={"serial", "port", "open"},
    annotations={
        "title": "Open Port",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "connection"},
)
def open_port(
    port_name: str,
    baud_rate: int = 9600,
    parity: int = 0,
    data_bits: int = 8,
    stop_bits: int = 0,
    flow_control: int = 0,
    read_buffer_size: int = 4096,
) -> Dict[str, object]:
    """Open a serial port.

    port_name: Port name (e.g. 'COM3' on Windows, '/dev/ttyUSB0' on Linux)
    baud_rate: Baud rate (e.g. 9600, 115200, 921600)
    parity: 0=None, 1=Odd, 2=Even, 3=Mark, 4=Space
    data_bits: 5, 6, 7, or 8
    stop_bits: 0=One, 1=OneAndHalf, 2=Two
    flow_control: 0=None, 1=Hardware, 2=Software
    read_buffer_size: Read buffer size in bytes
    """
    port_state.ensure_initialized()
    sp = port_state.get_or_create(port_name)

    try:
        sp.init(
            port_name=port_name,
            baud_rate=baud_rate,
            parity=Parity(parity),
            data_bits=DataBits(data_bits),
            stop_bits=StopBits(stop_bits),
            flow_control=FlowControl(flow_control),
            read_buffer_size=read_buffer_size,
        )
        ok = sp.open()
        if not ok:
            raise RuntimeError(
                f"Failed to open port '{port_name}': {sp.get_last_error_msg()}"
            )

        # Register on_read callback to buffer data
        sp.set_callback_on_read(
            lambda data: port_state._read_buffer.setdefault(port_name, []).append(
                {
                    "data_hex": data.hex(),
                    "data_utf8": data.decode("utf-8", errors="ignore"),
                }
            )
        )

        return {
            "message": f"Opened {port_name}",
            "port_name": port_name,
            "baud_rate": baud_rate,
        }
    except RuntimeError:
        raise
    except Exception as exc:
        raise RuntimeError(f"Failed to open port: {exc}") from exc


@mcp.tool(
    name="close_port",
    description="Close a serial port by port name.",
    tags={"serial", "port", "close"},
    annotations={
        "title": "Close Port",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "connection"},
)
def close_port(port_name: str) -> Dict[str, str]:
    """Close a serial port."""
    sp = port_state.ensure_port(port_name)
    try:
        sp.close()
        port_state.remove(port_name)
        return {"message": f"Closed {port_name}", "port_name": port_name}
    except Exception as exc:
        raise RuntimeError(f"Failed to close port: {exc}") from exc


@mcp.tool(
    name="port_info",
    description="Get status and current configuration of an open serial port.",
    tags={"serial", "port", "read", "status"},
    annotations={
        "title": "Port Info",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "discovery"},
)
def port_info(port_name: str) -> Dict[str, object]:
    """Get status and configuration of a serial port."""
    sp = port_state.ensure_port(port_name)
    return {
        "port_name": sp.get_port_name(),
        "is_open": sp.is_open(),
        "baud_rate": sp.get_baud_rate(),
        "parity": int(sp.get_parity()),
        "data_bits": int(sp.get_data_bits()),
        "stop_bits": int(sp.get_stop_bits()),
        "flow_control": int(sp.get_flow_control()),
        "read_buffer_size": sp.get_read_buffer_size(),
        "read_interval_timeout": sp.get_read_interval_timeout(),
        "last_error": sp.get_last_error(),
        "last_error_msg": sp.get_last_error_msg(),
    }


@mcp.tool(
    name="read_port",
    description="Read up to 'size' bytes from an open serial port. "
                "Returns hex and UTF-8 representations of the data.",
    tags={"serial", "port", "read"},
    annotations={
        "title": "Read Port",
        "readOnlyHint": True,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def read_port(port_name: str, size: int = 1024) -> Dict[str, object]:
    """Read data from a serial port.

    port_name: Port name
    size: Maximum number of bytes to read (default: 1024)
    """
    sp = port_state.ensure_port(port_name)
    if not sp.is_open():
        raise RuntimeError(f"Port '{port_name}' is not open")

    try:
        data = sp.read(size)
        return {
            "port_name": port_name,
            "size": len(data),
            "data_hex": data.hex(),
            "data_utf8": data.decode("utf-8", errors="ignore"),
        }
    except Exception as exc:
        raise RuntimeError(f"Read failed: {exc}") from exc


@mcp.tool(
    name="read_all_port",
    description="Read all available bytes from an open serial port.",
    tags={"serial", "port", "read"},
    annotations={
        "title": "Read All Port",
        "readOnlyHint": True,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def read_all_port(port_name: str) -> Dict[str, object]:
    """Read all available data from a serial port."""
    sp = port_state.ensure_port(port_name)
    if not sp.is_open():
        raise RuntimeError(f"Port '{port_name}' is not open")

    try:
        data = sp.read_all()
        return {
            "port_name": port_name,
            "size": len(data),
            "data_hex": data.hex(),
            "data_utf8": data.decode("utf-8", errors="ignore"),
        }
    except Exception as exc:
        raise RuntimeError(f"Read all failed: {exc}") from exc


@mcp.tool(
    name="write_port",
    description="Write data to an open serial port. Data must be a hex string "
                "(e.g., '48656C6C6F' for 'Hello').",
    tags={"serial", "port", "write"},
    annotations={
        "title": "Write Port",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def write_port(port_name: str, data: str) -> Dict[str, object]:
    """Write hex data to a serial port.

    port_name: Port name
    data: Hex string to write (e.g. '48656C6C6F')
    """
    sp = port_state.ensure_port(port_name)
    if not sp.is_open():
        raise RuntimeError(f"Port '{port_name}' is not open")

    try:
        data_bytes = bytes.fromhex(data)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc

    try:
        written = sp.write(data_bytes)
        return {"port_name": port_name, "bytes_written": written}
    except Exception as exc:
        raise RuntimeError(f"Write failed: {exc}") from exc


@mcp.tool(
    name="flush_port",
    description="Flush serial port buffers. Specify 'all', 'read', or 'write'.",
    tags={"serial", "port"},
    annotations={
        "title": "Flush Port",
        "readOnlyHint": False,
        "destructiveHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def flush_port(port_name: str, buffer: str = "all") -> Dict[str, object]:
    """Flush serial port buffers.

    port_name: Port name
    buffer: 'all' (default), 'read', or 'write'
    """
    sp = port_state.ensure_port(port_name)
    if not sp.is_open():
        raise RuntimeError(f"Port '{port_name}' is not open")

    try:
        if buffer == "read":
            ok = sp.flush_read_buffers()
        elif buffer == "write":
            ok = sp.flush_write_buffers()
        else:
            ok = sp.flush_buffers()

        return {"message": f"Flushed {buffer} buffers", "success": ok}
    except Exception as exc:
        raise RuntimeError(f"Flush failed: {exc}") from exc


@mcp.tool(
    name="configure_port",
    description="Update serial port configuration. "
                "Only provide the parameters you want to change.",
    tags={"serial", "port", "config"},
    annotations={
        "title": "Configure Port",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "config"},
)
def configure_port(
    port_name: str,
    baud_rate: Optional[int] = None,
    parity: Optional[int] = None,
    data_bits: Optional[int] = None,
    stop_bits: Optional[int] = None,
    flow_control: Optional[int] = None,
    read_buffer_size: Optional[int] = None,
    read_interval_timeout: Optional[int] = None,
    dtr: Optional[bool] = None,
    rts: Optional[bool] = None,
) -> Dict[str, object]:
    """Update serial port configuration.

    Only provide the parameters you want to change.
    parity: 0=None, 1=Odd, 2=Even, 3=Mark, 4=Space
    data_bits: 5, 6, 7, or 8
    stop_bits: 0=One, 1=OneAndHalf, 2=Two
    flow_control: 0=None, 1=Hardware, 2=Software
    dtr/rts: Set or clear DTR/RTS lines
    """
    sp = port_state.ensure_port(port_name)

    try:
        if baud_rate is not None:
            sp.set_baud_rate(baud_rate)
        if parity is not None:
            sp.set_parity(parity)
        if data_bits is not None:
            sp.set_data_bits(data_bits)
        if stop_bits is not None:
            sp.set_stop_bits(stop_bits)
        if flow_control is not None:
            sp.set_flow_control(flow_control)
        if read_buffer_size is not None:
            sp.set_read_buffer_size(read_buffer_size)
        if read_interval_timeout is not None:
            sp.set_read_interval_timeout(read_interval_timeout)
        if dtr is not None:
            sp.set_dtr(dtr)
        if rts is not None:
            sp.set_rts(rts)

        return {"message": "Configuration updated", "port_name": port_name}
    except Exception as exc:
        raise RuntimeError(f"Config update failed: {exc}") from exc


@mcp.tool(
    name="set_port_callback_on_read",
    description="Register a callback for incoming data on a serial port. "
                "Data will be buffered and can be retrieved with get_buffered_data.",
    tags={"serial", "port", "callback"},
    annotations={
        "title": "Set Port Read Callback",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "callback"},
)
def set_port_callback_on_read(port_name: str) -> Dict[str, str]:
    """Start buffering incoming data from a serial port.

    Incoming data will be buffered and can be retrieved with get_buffered_data.
    """
    sp = port_state.ensure_port(port_name)
    if not sp.is_open():
        raise RuntimeError(f"Port '{port_name}' is not open")

    try:
        sp.set_callback_on_read(
            lambda data: port_state._read_buffer.setdefault(port_name, []).append(
                {
                    "data_hex": data.hex(),
                    "data_utf8": data.decode("utf-8", errors="ignore"),
                }
            )
        )
        return {"message": f"Read callback set for {port_name}", "port_name": port_name}
    except Exception as exc:
        raise RuntimeError(f"Failed to set read callback: {exc}") from exc


@mcp.tool(
    name="get_buffered_data",
    description="Retrieve and clear all buffered incoming data for a serial port.",
    tags={"serial", "port", "read"},
    annotations={
        "title": "Get Buffered Data",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def get_buffered_data(port_name: str) -> List[Dict[str, str]]:
    """Return all buffered incoming data and clear the buffer."""
    if port_name not in port_state._read_buffer:
        return []

    samples = port_state._read_buffer[port_name]
    port_state._read_buffer[port_name] = []
    return samples


def main() -> None:
    parser = argparse.ArgumentParser(description="SimpleCommKitAiSerialPort MCP Server")
    parser.add_argument(
        "--transport",
        default="stdio",
        choices=["stdio", "http", "sse", "streamable-http"],
        help=(
            "Transport protocol: 'stdio' (local), 'streamable-http' (recommended for remote),"
            " 'http' or 'sse' (legacy, may show init warnings)"
        ),
    )
    parser.add_argument("--host", default="127.0.0.1", help="Host to bind to")
    parser.add_argument("--port", type=int, default=8001, help="Port to bind to")
    args = parser.parse_args()

    # Ensure PortState is initialized before the MCP server starts accepting requests.
    # This prevents "Received request before initialization was complete" warnings
    # from FastMCP when a client connects too quickly in http/sse transport modes.
    port_state.init()

    if args.transport in ("http", "sse", "streamable-http"):
        mcp.run(transport=args.transport, host=args.host, port=args.port)
    else:
        mcp.run()


if __name__ == "__main__":
    main()
