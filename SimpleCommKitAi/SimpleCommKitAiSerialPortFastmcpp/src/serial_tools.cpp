#include "serial_state.hpp"

#include <fastmcpp/app.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace SimpleCommKitAiSerialPortFastmcpp
{

// ===========================================================================
// Schema builders — use nlohmann::json operator[] instead of initializer lists
// or raw string literals, both of which MSVC (locale 936) struggles with.
// ===========================================================================

static fastmcpp::Json prop_string(const char* desc, const char* default_val = nullptr)
{
    fastmcpp::Json p;
    p["type"] = "string";
    p["description"] = desc;
    if (default_val)
        p["default"] = default_val;
    return p;
}

static fastmcpp::Json prop_integer(const char* desc, int default_val = 0)
{
    fastmcpp::Json p;
    p["type"] = "integer";
    p["description"] = desc;
    p["default"] = default_val;
    return p;
}

static fastmcpp::Json prop_boolean(const char* desc, bool default_val)
{
    fastmcpp::Json p;
    p["type"] = "boolean";
    p["description"] = desc;
    p["default"] = default_val;
    return p;
}

static fastmcpp::Json make_object_schema(std::initializer_list<std::pair<const char*, fastmcpp::Json>> props,
                                         std::initializer_list<const char*> required = {})
{
    fastmcpp::Json schema;
    schema["type"] = "object";
    for (auto& kv : props)
        schema["properties"][kv.first] = kv.second;
    if (required.size() > 0)
        schema["required"] = required;
    return schema;
}

// ===========================================================================
// Helper: build a port-config JSON response from a SimpleCommKitSerialPort.
// ===========================================================================
static fastmcpp::Json build_config_json(SimpleCommKit::SimpleCommKitSerialPort& sp,
                                        const std::string& port_name)
{
    fastmcpp::Json cfg;
    cfg["port_name"] = port_name;
    cfg["baud_rate"] = sp.get_Baud_Rate();
    cfg["parity"] = parity_name(sp.get_Parity());
    cfg["data_bits"] = static_cast<int>(sp.get_Data_Bits());
    cfg["stop_bits"] = stop_bits_name(sp.get_Stop_Bits());
    cfg["flow_control"] = flow_control_name(sp.get_Flow_Control());
    cfg["read_buffer_size"] = static_cast<int>(sp.get_Read_Buffer_Size());
    cfg["read_interval_timeout_ms"] = static_cast<int>(sp.get_Read_Interval_Timeout());
    cfg["is_open"] = sp.is_Open();
    return cfg;
}

// ===========================================================================
// Register all 14 serial-port MCP tools on a FastMCP application instance.
// ===========================================================================

void register_tools(fastmcpp::FastMCP& app)
{
    using Json = fastmcpp::Json;

    // -----------------------------------------------------------------------
    // 1. get_available_ports
    // -----------------------------------------------------------------------
    app.tool(
        "get_available_ports",
        [](const Json&) -> Json
        {
            auto ports = SimpleCommKit::SimpleCommKitSerialPort::get_Available_Ports();

            Json arr = Json::array();
            for (const auto& p : ports)
                arr.push_back(port_info_to_json(p));

            return Json{{"content",
                         Json::array({Json{{"type", "text"}, {"text", arr.dump(2)}}})}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "List all available serial (COM) ports on the host system. "
            "Returns port_name, description, and hardware_id for each port."});

    // -----------------------------------------------------------------------
    // 2. open
    // -----------------------------------------------------------------------
    app.tool(
        "open",
        make_object_schema({
            {"port_name",            prop_string ("Serial port name (e.g. 'COM3', '/dev/ttyUSB0')")},
            {"baud_rate",            prop_integer("Baud rate (default 115200)", 115200)},
            {"parity",               prop_string ("Parity: none, odd, even, mark, space (default none)", "none")},
            {"data_bits",            prop_integer("Data bits: 5, 6, 7, 8 (default 8)", 8)},
            {"stop_bits",            prop_string ("Stop bits: one, one_and_half, two (default one)", "one")},
            {"flow_control",         prop_string ("Flow control: none, hardware, software (default none)", "none")},
            {"read_buffer_size",     prop_integer("Read buffer size in bytes (default 4096)", 4096)},
            {"read_interval_timeout_ms", prop_integer("Read interval timeout in ms (default 100)", 100)},
        }, {"port_name"}),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();

            std::string port_name = input.value("port_name", "");
            int baud_rate = input.value("baud_rate", 115200);
            std::string parity_str = input.value("parity", "none");
            int data_bits = input.value("data_bits", 8);
            std::string stop_bits_str = input.value("stop_bits", "one");
            std::string flow_ctrl_str = input.value("flow_control", "none");
            unsigned int read_buf_size = static_cast<unsigned int>(input.value("read_buffer_size", 4096));
            unsigned int read_timeout = static_cast<unsigned int>(input.value("read_interval_timeout_ms", 100));

            // Check if already open
            if (ss.has_port(port_name))
            {
                auto* existing = ss.get_port(port_name);
                if (existing && existing->is_Open())
                    return Json{{"message", "Port already open: " + port_name},
                                {"port_name", port_name}};
            }

            // Parse enum strings
            auto parity = parse_parity(parity_str);
            auto stop_bits = parse_stop_bits(stop_bits_str);
            auto flow_ctrl = parse_flow_control(flow_ctrl_str);

            auto* sp = ss.get_or_create_port(port_name);

            sp->init(port_name, baud_rate, parity,
                     static_cast<SimpleCommKit::DataBits>(data_bits),
                     stop_bits, flow_ctrl, read_buf_size);

            if (read_timeout > 0)
                sp->set_Read_Interval_Timeout(read_timeout);

            // Install read callback before opening
            ss.setup_read_callback(port_name);

            if (!sp->open())
            {
                int err = sp->get_Last_Error();
                std::string err_msg = sp->get_Last_Error_Msg();
                throw std::runtime_error("Failed to open " + port_name +
                                         " (error " + std::to_string(err) + ": " + err_msg + ")");
            }

            return Json{{"message", "Opened serial port: " + port_name},
                        {"port_name", port_name},
                        {"baud_rate", baud_rate},
                        {"parity", parity_str},
                        {"data_bits", data_bits},
                        {"stop_bits", stop_bits_str},
                        {"flow_control", flow_ctrl_str}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Open a serial port for communication. "
            "Configure baud rate, parity, data bits, stop bits, and flow control. "
            "A read callback is automatically installed to buffer incoming data."});

    // -----------------------------------------------------------------------
    // 3. close
    // -----------------------------------------------------------------------
    app.tool(
        "close",
        make_object_schema({
            {"port_name", prop_string("Port name to close (omitted = close all)", "")},
        }),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();
            std::string port_name = input.value("port_name", "");

            if (!port_name.empty())
            {
                auto* sp = ss.get_port(port_name);
                if (sp && sp->is_Open())
                    sp->close();
                ss.remove_port(port_name);
                return Json{{"message", "Closed serial port: " + port_name}};
            }
            else
            {
                // Close all
                ss.remove_all_ports();
                return Json{{"message", "Closed all serial ports"}};
            }
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Close a serial port. Provide a port_name to close a specific port, "
            "or omit to close all open ports."});

    // -----------------------------------------------------------------------
    // 4. write
    // -----------------------------------------------------------------------
    app.tool(
        "write",
        make_object_schema({
            {"port_name", prop_string("Name of the open serial port")},
            {"data",      prop_string("Hex string to write (e.g. '00FF' or '01 02 AA')")},
        }, {"port_name", "data"}),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();
            std::string port_name = input.value("port_name", "");
            std::string data_hex = input.value("data", "");

            auto* sp = ss.get_port(port_name);
            if (!sp)
                throw std::runtime_error("Port not found: " + port_name);
            if (!sp->is_Open())
                throw std::runtime_error("Port not open: " + port_name);

            auto data_bytes = hex_to_bytes(data_hex);
            int written = sp->write(data_bytes);

            if (written < 0)
                throw std::runtime_error("Write failed on " + port_name);

            return Json{{"message", "Wrote " + std::to_string(written) + " byte(s)"},
                        {"port_name", port_name},
                        {"bytes_written", written}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Write hex data to an open serial port. "
            "Data must be a hex string (e.g. '48 65 6C 6C 6F' for 'Hello')."});

    // -----------------------------------------------------------------------
    // 5. read
    // -----------------------------------------------------------------------
    app.tool(
        "read",
        make_object_schema({
            {"port_name", prop_string ("Name of the open serial port")},
            {"size",      prop_integer("Maximum bytes to read (default 1024)", 1024)},
        }, {"port_name"}),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();
            std::string port_name = input.value("port_name", "");
            int size = input.value("size", 1024);

            auto* sp = ss.get_port(port_name);
            if (!sp)
                throw std::runtime_error("Port not found: " + port_name);
            if (!sp->is_Open())
                throw std::runtime_error("Port not open: " + port_name);

            auto data = sp->read(size);

            return Json{{"port_name", port_name},
                        {"data_hex", bytes_to_hex(data)},
                        {"data_utf8", bytes_to_utf8_safe(data)},
                        {"data_length", data.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Read up to `size` bytes from a serial port (synchronous). "
            "Returns data as hex and UTF-8 strings."});

    // -----------------------------------------------------------------------
    // 6. read_all
    // -----------------------------------------------------------------------
    app.tool(
        "read_all",
        make_object_schema({
            {"port_name", prop_string("Name of the open serial port")},
        }, {"port_name"}),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();
            std::string port_name = input.value("port_name", "");

            auto* sp = ss.get_port(port_name);
            if (!sp)
                throw std::runtime_error("Port not found: " + port_name);
            if (!sp->is_Open())
                throw std::runtime_error("Port not open: " + port_name);

            auto data = sp->read_All();

            return Json{{"port_name", port_name},
                        {"data_hex", bytes_to_hex(data)},
                        {"data_utf8", bytes_to_utf8_safe(data)},
                        {"data_length", data.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Read all currently available data from a serial port's input buffer (synchronous). "
            "Returns data as hex and UTF-8 strings."});

    // -----------------------------------------------------------------------
    // 7. get_config
    // -----------------------------------------------------------------------
    app.tool(
        "get_config",
        make_object_schema({
            {"port_name", prop_string("Name of the open serial port")},
        }, {"port_name"}),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();
            std::string port_name = input.value("port_name", "");

            auto* sp = ss.get_port(port_name);
            if (!sp)
                throw std::runtime_error("Port not found: " + port_name);

            return build_config_json(*sp, port_name);
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Get the current configuration of a serial port (baud rate, "
            "parity, data bits, stop bits, flow control, buffer size, timeout)."});

    // -----------------------------------------------------------------------
    // 8. set_config
    // -----------------------------------------------------------------------
    app.tool(
        "set_config",
        make_object_schema({
            {"port_name",             prop_string ("Name of the open serial port")},
            {"baud_rate",             prop_integer("New baud rate (0 = no change)")},
            {"parity",                prop_string ("New parity: none/odd/even/mark/space (empty = no change)", "")},
            {"data_bits",             prop_integer("New data bits: 5/6/7/8 (0 = no change)")},
            {"stop_bits",             prop_string ("New stop bits: one/one_and_half/two (empty = no change)", "")},
            {"flow_control",          prop_string ("New flow control: none/hardware/software (empty = no change)", "")},
            {"read_buffer_size",      prop_integer("New read buffer size (0 = no change)")},
            {"read_interval_timeout_ms", prop_integer("New read interval timeout in ms (-1 = no change)", -1)},
        }, {"port_name"}),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();
            std::string port_name = input.value("port_name", "");

            auto* sp = ss.get_port(port_name);
            if (!sp)
                throw std::runtime_error("Port not found: " + port_name);

            int baud_rate = input.value("baud_rate", 0);
            std::string parity_str = input.value("parity", "");
            int data_bits = input.value("data_bits", 0);
            std::string stop_bits_str = input.value("stop_bits", "");
            std::string flow_ctrl_str = input.value("flow_control", "");
            int buf_size = input.value("read_buffer_size", 0);
            int timeout = input.value("read_interval_timeout_ms", -1);

            if (baud_rate > 0)
                sp->set_Baud_Rate(baud_rate);
            if (!parity_str.empty())
                sp->set_Parity(parse_parity(parity_str));
            if (data_bits > 0)
                sp->set_Data_Bits(static_cast<SimpleCommKit::DataBits>(data_bits));
            if (!stop_bits_str.empty())
                sp->set_Stop_Bits(parse_stop_bits(stop_bits_str));
            if (!flow_ctrl_str.empty())
                sp->set_Flow_Control(parse_flow_control(flow_ctrl_str));
            if (buf_size > 0)
                sp->set_Read_Buffer_Size(static_cast<unsigned int>(buf_size));
            if (timeout >= 0)
                sp->set_Read_Interval_Timeout(static_cast<unsigned int>(timeout));

            return build_config_json(*sp, port_name);
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Update configuration of an open serial port. "
            "Only non-zero/non-empty values will be changed."});

    // -----------------------------------------------------------------------
    // 9. flush_buffers
    // -----------------------------------------------------------------------
    app.tool(
        "flush_buffers",
        make_object_schema({
            {"port_name", prop_string("Name of the open serial port")},
            {"direction", prop_string("Which buffer to flush: both (default), read, write", "both")},
        }, {"port_name"}),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();
            std::string port_name = input.value("port_name", "");
            std::string direction = input.value("direction", "both");

            auto* sp = ss.get_port(port_name);
            if (!sp)
                throw std::runtime_error("Port not found: " + port_name);
            if (!sp->is_Open())
                throw std::runtime_error("Port not open: " + port_name);

            bool ok = true;
            if (direction == "read")
                ok = sp->flush_Read_Buffers();
            else if (direction == "write")
                ok = sp->flush_Write_Buffers();
            else
                ok = sp->flush_Buffers();

            if (!ok)
                throw std::runtime_error("Flush failed on " + port_name);

            return Json{{"message", "Flushed " + direction + " buffer(s) on " + port_name},
                        {"port_name", port_name}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Flush the read and/or write buffers of a serial port. "
            "Use 'read', 'write', or 'both' (default) for direction."});

    // -----------------------------------------------------------------------
    // 10. set_dtr
    // -----------------------------------------------------------------------
    app.tool(
        "set_dtr",
        make_object_schema({
            {"port_name", prop_string ("Name of the open serial port")},
            {"set",       prop_boolean("True to assert DTR, false to de-assert (default true)", true)},
        }, {"port_name"}),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();
            std::string port_name = input.value("port_name", "");
            bool set = input.value("set", true);

            auto* sp = ss.get_port(port_name);
            if (!sp)
                throw std::runtime_error("Port not found: " + port_name);
            if (!sp->is_Open())
                throw std::runtime_error("Port not open: " + port_name);

            sp->set_Dtr(set);

            return Json{{"message", std::string("DTR ") + (set ? "asserted" : "de-asserted") + " on " + port_name},
                        {"port_name", port_name},
                        {"dtr", set}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Control the DTR (Data Terminal Ready) signal line on a serial port."});

    // -----------------------------------------------------------------------
    // 11. set_rts
    // -----------------------------------------------------------------------
    app.tool(
        "set_rts",
        make_object_schema({
            {"port_name", prop_string ("Name of the open serial port")},
            {"set",       prop_boolean("True to assert RTS, false to de-assert (default true)", true)},
        }, {"port_name"}),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();
            std::string port_name = input.value("port_name", "");
            bool set = input.value("set", true);

            auto* sp = ss.get_port(port_name);
            if (!sp)
                throw std::runtime_error("Port not found: " + port_name);
            if (!sp->is_Open())
                throw std::runtime_error("Port not open: " + port_name);

            sp->set_Rts(set);

            return Json{{"message", std::string("RTS ") + (set ? "asserted" : "de-asserted") + " on " + port_name},
                        {"port_name", port_name},
                        {"rts", set}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Control the RTS (Request To Send) signal line on a serial port."});

    // -----------------------------------------------------------------------
    // 12. get_open_ports
    // -----------------------------------------------------------------------
    app.tool(
        "get_open_ports",
        [](const Json&) -> Json
        {
            auto& ss = SerialState::instance();

            auto names = ss.get_port_names();
            Json arr = Json::array();

            for (const auto& name : names)
            {
                auto* sp = ss.get_port(name);
                if (sp)
                {
                    arr.push_back(Json{
                        {"port_name", name},
                        {"is_open", sp->is_Open()},
                    });
                }
            }

            return Json{{"open_ports", arr},
                        {"open_count", names.size()}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "List currently open/managed serial ports and their status."});

    // -----------------------------------------------------------------------
    // 13. get_read_data
    // -----------------------------------------------------------------------
    app.tool(
        "get_read_data",
        make_object_schema({
            {"port_name", prop_string("Name of the open serial port to read from")},
        }, {"port_name"}),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();
            std::string port_name = input.value("port_name", "");

            auto entries = ss.drain_read_buffer(port_name);

            Json arr = Json::array();
            for (const auto& e : entries)
            {
                arr.push_back(Json{{"port_name", e.port_name},
                                   {"data_hex", e.data_hex},
                                   {"data_utf8", e.data_utf8},
                                   {"data_length", e.data_length}});
            }
            return Json{{"content",
                         Json::array({Json{{"type", "text"}, {"text", arr.dump(2)}}})}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Retrieve and clear buffered read data for a serial port. "
            "Data is buffered automatically via the On_Read callback after opening a port."});

    // -----------------------------------------------------------------------
    // 14. get_error
    // -----------------------------------------------------------------------
    app.tool(
        "get_error",
        make_object_schema({
            {"port_name", prop_string("Name of the open serial port")},
        }, {"port_name"}),
        [](const Json& input) -> Json
        {
            auto& ss = SerialState::instance();
            std::string port_name = input.value("port_name", "");

            auto* sp = ss.get_port(port_name);
            if (!sp)
                throw std::runtime_error("Port not found: " + port_name);

            int err_code = sp->get_Last_Error();
            std::string err_msg = sp->get_Last_Error_Msg();
            std::string err_desc =
                SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(
                    static_cast<SimpleCommKit::ErrorCode>(err_code));

            sp->clear_Error();

            return Json{{"port_name", port_name},
                        {"error_code", err_code},
                        {"error_message", err_msg},
                        {"error_description", err_desc}};
        },
        fastmcpp::FastMCP::ToolOptions{
            std::nullopt, std::nullopt,
            "Get and clear the last error from a serial port. "
            "Useful for diagnosing communication issues."});
}

} // namespace SimpleCommKitAiSerialPortFastmcpp
