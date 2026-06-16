"""
SimpleCommKitAiUsb HTTP Server

REST API for remote USB device control using FastAPI.
"""

import os
import sys
import time
import signal
import asyncio
import logging
import threading
from typing import List, Optional, Dict
from queue import Queue, Empty

import uvicorn
from fastapi import FastAPI, HTTPException, Query
from fastapi.responses import StreamingResponse
from pydantic import BaseModel, Field

from SimpleCommKitPyUsb import (
from SimpleCommKitAiUsb import __version__
    SimpleCommKitUsb,
    UsbDeviceInfo,
    get_error_description,
)

logger = logging.getLogger(__name__)


# ============================================================
# Pydantic models
# ============================================================

class OpenRequest(BaseModel):
    vid: str = Field("", description="Vendor ID in hex (e.g. '0x1234')")
    pid: str = Field("", description="Product ID in hex")
    serial: str = Field("", description="Serial number")

class TransferRequest(BaseModel):
    endpoint: str = Field(..., description="Endpoint address in hex (e.g. '0x01')")
    data: str = Field("", description="Hex data string (e.g. 'AB CD EF')")
    timeout: int = Field(1000, description="Timeout in milliseconds")

class ControlRequest(BaseModel):
    bm_request_type: str = Field(..., description="bmRequestType in hex (e.g. '0x80')")
    b_request: str = Field(..., description="bRequest in hex (e.g. '0x06')")
    w_value: str = Field(..., description="wValue in hex (e.g. '0x0100')")
    w_index: str = Field("0x0000", description="wIndex in hex")
    data: str = Field("", description="Hex data string")
    length: int = Field(64, description="Data length for IN transfers")
    timeout: int = Field(1000, description="Timeout in milliseconds")

class ReadPollRequest(BaseModel):
    path: str = Field(..., description="Device path")
    endpoint: str = Field(..., description="Endpoint address in hex")

class HotplugRequest(BaseModel):
    vid: str = Field("0", description="Vendor ID in hex (0 = any)")
    pid: str = Field("0", description="Product ID in hex (0 = any)")

class DeviceInfoResponse(BaseModel):
    path: str
    vendor_id: str
    product_id: str
    manufacturer_string: str
    product_string: str
    serial_number: str
    bus_number: int
    device_address: int

    @classmethod
    def from_cpp(cls, info: UsbDeviceInfo) -> "DeviceInfoResponse":
        return cls(
            path=info.path,
            vendor_id=f"0x{info.vendor_id:04X}",
            product_id=f"0x{info.product_id:04X}",
            manufacturer_string=info.manufacturer_string,
            product_string=info.product_string,
            serial_number=info.serial_number,
            bus_number=info.bus_number,
            device_address=info.device_address,
        )


# ============================================================
# Global state
# ============================================================

class UsbState:
    def __init__(self):
        self.usb = SimpleCommKitUsb()
        self.read_queue: Queue = Queue()
        self.read_event = threading.Event()
        self._init_done = False

    def ensure_init(self):
        if not self._init_done:
            self.usb.init()
            self.usb.set_Callback_On_Read(self._on_read)
            self.usb.set_Callback_Error(
                lambda code: logger.error("[USB Error] %s: %s", code, get_error_description(code))
            )
            self._init_done = True

    def _on_read(self, dev_info: UsbDeviceInfo, data: bytes):
        self.read_queue.put({
            "path": dev_info.path,
            "data": data,
            "timestamp": time.time(),
        })
        self.read_event.set()

    def close(self):
        self.usb.stop_Hotplug()
        self.usb.stop_Read_Poll()
        self.usb.close()
        self.usb.exit()
        self._init_done = False


state = UsbState()
app = FastAPI(title="SimpleCommKitAiUsb HTTP API", version=__version__)


def _parse_hex(s: str) -> int:
    return int(s, 16)

def _parse_hex_bytes(s: str) -> bytes:
    return bytes.fromhex(s.replace(" ", "").replace(",", ""))


# ============================================================
# Endpoints
# ============================================================

@app.get("/")
async def root():
    return {"service": "SimpleCommKitAiUsb", "version": "0.1.0"}


@app.get("/devices")
async def list_devices(
    vendor_id: str = Query("0", description="Filter by vendor ID (hex)"),
    product_id: str = Query("0", description="Filter by product ID (hex)"),
):
    state.ensure_init()
    vid = _parse_hex(vendor_id)
    pid = _parse_hex(product_id)
    devices = SimpleCommKitUsb.get_available_devices(vid, pid)
    return [DeviceInfoResponse.from_cpp(d).model_dump() for d in devices]


@app.post("/open/{path}")
async def open_device(path: str):
    state.ensure_init()
    if state.usb.open(path):
        return {"status": "ok", "path": path}
    raise HTTPException(400, "Failed to open device")


@app.post("/open")
async def open_device_by_vidpid(req: OpenRequest):
    state.ensure_init()
    vid = _parse_hex(req.vid)
    pid = _parse_hex(req.pid)
    if state.usb.open(vid, pid, req.serial):
        return {"status": "ok", "vid": req.vid, "pid": req.pid}
    raise HTTPException(400, "Failed to open device")


@app.post("/close/{path}")
async def close_device(path: str):
    state.usb.close(path)
    return {"status": "ok", "path": path}


@app.post("/close")
async def close_all():
    state.usb.close()
    return {"status": "ok"}


@app.get("/open_paths")
async def open_paths():
    return {"paths": state.usb.get_Open_Paths()}


@app.post("/device/{path}/claim")
async def claim_interface(path: str, interface_number: int = 0):
    if state.usb.claim_interface(path, interface_number):
        return {"status": "ok"}
    raise HTTPException(400, "Failed to claim interface")


@app.post("/device/{path}/release")
async def release_interface(path: str, interface_number: int = 0):
    if state.usb.release_interface(path, interface_number):
        return {"status": "ok"}
    raise HTTPException(400, "Failed to release interface")


@app.post("/device/{path}/control")
async def control_transfer(path: str, req: ControlRequest):
    bm = _parse_hex(req.bm_request_type)
    b = _parse_hex(req.b_request)
    wv = _parse_hex(req.w_value)
    wi = _parse_hex(req.w_index)
    data = _parse_hex_bytes(req.data) if req.data else bytes(req.length)
    result = state.usb.control_transfer(path, bm, b, wv, wi, data, req.timeout)
    return {"status": "ok", "length": len(result), "data": result.hex()}


@app.post("/device/{path}/bulk_out")
async def bulk_out(path: str, req: TransferRequest):
    ep = _parse_hex(req.endpoint)
    data = _parse_hex_bytes(req.data)
    transferred = state.usb.bulk_transfer_out(path, ep, data, req.timeout)
    return {"status": "ok", "transferred": transferred}


@app.post("/device/{path}/bulk_in")
async def bulk_in(path: str, endpoint: str = Query(...),
                   length: int = Query(64), timeout: int = Query(1000)):
    ep = _parse_hex(endpoint)
    data = state.usb.bulk_transfer_in(path, ep, length, timeout)
    return {"status": "ok", "length": len(data), "data": data.hex()}


@app.post("/device/{path}/intr_out")
async def intr_out(path: str, req: TransferRequest):
    ep = _parse_hex(req.endpoint)
    data = _parse_hex_bytes(req.data)
    transferred = state.usb.interrupt_transfer_out(path, ep, data, req.timeout)
    return {"status": "ok", "transferred": transferred}


@app.post("/device/{path}/intr_in")
async def intr_in(path: str, endpoint: str = Query(...),
                   length: int = Query(64), timeout: int = Query(1000)):
    ep = _parse_hex(endpoint)
    data = state.usb.interrupt_transfer_in(path, ep, length, timeout)
    return {"status": "ok", "length": len(data), "data": data.hex()}


@app.post("/read/start")
async def start_read(req: ReadPollRequest):
    ep = _parse_hex(req.endpoint)
    state.usb.start_Read_Poll(req.path, ep)
    return {"status": "ok", "path": req.path, "endpoint": req.endpoint}


@app.post("/read/stop")
async def stop_read(path: str = Query(...)):
    state.usb.stop_Read_Poll(path)
    return {"status": "ok", "path": path}


@app.get("/read/stream")
async def read_stream():
    async def event_generator():
        while True:
            state.read_event.wait(timeout=5.0)
            state.read_event.clear()
            while True:
                try:
                    msg = state.read_queue.get_nowait()
                    yield f"data: {msg['path']} {msg['data'].hex()}\n\n"
                except Empty:
                    break
    return StreamingResponse(event_generator(), media_type="text/event-stream")


@app.post("/hotplug/start")
async def start_hotplug(req: HotplugRequest):
    vid = _parse_hex(req.vid) if req.vid != "0" else 0
    pid = _parse_hex(req.pid) if req.pid != "0" else 0
    state.usb.start_Hotplug(vid, pid)
    return {"status": "ok"}


@app.post("/hotplug/stop")
async def stop_hotplug():
    state.usb.stop_Hotplug()
    return {"status": "ok"}


@app.get("/hotplug/status")
async def hotplug_status():
    return {"active": state.usb.is_Hotplug_Active()}


def main():
    import argparse
parser = argparse.ArgumentParser(description="SimpleCommKitAiUsb HTTP Server")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--log-level", default="info")
    args = parser.parse_args()

    logging.basicConfig(level=args.log_level.upper())

    def cleanup(sig, frame):
        state.close()
        sys.exit(0)
    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)

    uvicorn.run(app, host=args.host, port=args.port)


if __name__ == "__main__":
    main()
