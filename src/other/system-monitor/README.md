# Prototype Server for banksia system status

## Usage
Start server and open browser window:
`$ cd ./server/src`
`$ python3 main.py`

`$ xdg-open https://localhost:5000`

## Testing
Start virtual can interface:
`../JCAN/utils/mk-vcan.sh`

Start board faker:
`cd ./test_client`
`python3 main.py`

## Notes
- Uses `0x5**` CAN IDs. lower can IDs have higher priority and every other CAN message is either `0x4**` or `0x0**`, so CAN messages for system status will not interrupt normal operation.
