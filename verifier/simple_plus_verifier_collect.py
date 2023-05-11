import os, sys, socket, hmac, hashlib, time

# Definitions from sa_shared.h
SIMPLE_KEY_SIZE         = 32
SIMPLE_HMAC_LEN         = 32

# Structure of SIMPLE+ collect_req, from sa_simple_plus.h
SIMPLE_PLUS_COLLECTREQ_TIMEOUT_LEN  = 2
SIMPLE_PLUS_COLLECTREQ_HMAC_LEN     = SIMPLE_HMAC_LEN
SIMPLE_PLUS_COLLECTREQ_LEN          = (SIMPLE_PLUS_COLLECTREQ_TIMEOUT_LEN + SIMPLE_PLUS_COLLECTREQ_HMAC_LEN)

# Command codes for prover TCP server
CMD_NODE_NAME           = 0x01.to_bytes(1, byteorder="little")
CMD_CLOSE_CONN          = 0x06.to_bytes(1, byteorder="little")
CMD_SIMPLE_PLUS_COLLECT = 0x08.to_bytes(1, byteorder="little")

# Command codes for internal SIMPLE+ messages
CMD_SIMPLE_PLUS_COLLECT_ACK     = 0xF0.to_bytes(1, byteorder="little")
CMD_SIMPLE_PLUS_COLLECT_REPORT  = 0xF1.to_bytes(1, byteorder="little")

# Get NODE_ID and TIMEOUT_MS from CLI args
if len(sys.argv) != 3:
    print("Usage: python3 simple_verifier.py NODE_ID TIMEOUT_MS")
    sys.exit(1)

NODE_ID = None
try :
    NODE_ID = int(sys.argv[1])
except Exception:
    print("NODE_ID must be an integer")
    sys.exit(1)

TIMEOUT_MS = None
try :
    TIMEOUT_MS = int(sys.argv[2])
except Exception:
    print("TIMEOUT_MS must be an integer")
    sys.exit(1)

# Generate host IP
HOST = f"192.168.{NODE_ID}.1"
PORT = 3333

# Make counters directory
os.makedirs("counters", exist_ok=True)

# Function to get k_col (collect phase only needs to get the key)
# Stored in a file as it is updated in attestation phase
# k_col is initialised from keys/k_col.bin and can be reset using reset_counters.py
def get_k_col() -> bytes:
    try:
        with open("counters/simple_plus_k_col.bin", "rb") as f:
            return f.read()
        
    except FileNotFoundError:
        with open("keys/k_col.bin", "rb") as f:
            return f.read()
        
# Connect to node
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    print(f"Connecting to node {NODE_ID} via {HOST}:{PORT}")
    s.connect((HOST, PORT))

    # Transmit name or our connection will be rejected
    node_name = b'VERIFIER\0'
    s.send(CMD_NODE_NAME + len(node_name).to_bytes(1, byteorder="little") + node_name)

    ### Algorithm as per Figure 5 of SIMPLE paper ###
    print("Executing SIMPLE+ Collection Phase...")

    # Compute HMAC and construct collect_req
    data = TIMEOUT_MS.to_bytes(2, byteorder="little")
    k_col = get_k_col()
    h = hmac.digest(k_col, data, hashlib.sha256)
    collect_req = data + h

    # Send collect_req
    print(f"Sending collection request...")
    start_time = time.perf_counter_ns()
    s.send(CMD_SIMPLE_PLUS_COLLECT + len(collect_req).to_bytes(2, byteorder='little') + collect_req)

    # Receive ACK (we don't need to do anything with it)
    print("Waiting for ACK...")
    ack = s.recv(1 + SIMPLE_HMAC_LEN)
    if ack[:1] != CMD_SIMPLE_PLUS_COLLECT_ACK:
        print(f"Invalid command: 0x{ack.hex()}")
        sys.exit(1)

    print("Received ACK")

    local_ack_hmac = hmac.digest(k_col, ack[:1], hashlib.sha256)
    if hmac.compare_digest(ack[1:1+SIMPLE_HMAC_LEN], local_ack_hmac):
        print("ACK HMAC verified")
    else:
        print("ACK HMAC mismatch")
        sys.exit(1)

    # Receive report
    print("Waiting for report...")
    cmd = s.recv(1)
    stop_time = time.perf_counter_ns()
    if cmd != CMD_SIMPLE_PLUS_COLLECT_REPORT:
        print(f"Invalid command: 0x{cmd.hex()}")
        sys.exit(1)

    report_len = int.from_bytes(s.recv(2), byteorder="little")
    print(f"Report contains data for up to {report_len * 8} nodes")

    report = s.recv(report_len)
    if len(report) != report_len:
        print(f"Received report too short: {len(report)}")
        sys.exit(1)

    report_hmac = s.recv(SIMPLE_HMAC_LEN)
    if len(report_hmac) != SIMPLE_HMAC_LEN:
        print(f"Received report HMAC too short: {len(report_hmac)}")
        sys.exit(1)

    local_report_hmac = hmac.digest(k_col, cmd + report_len.to_bytes(2, byteorder='little') + report, hashlib.sha256)
    if hmac.compare_digest(report_hmac, local_report_hmac):
        print("Report HMAC verified")
    else:
        print("Report HMAC mismatch")
        sys.exit(1)

    print(f"Received report in {(stop_time - start_time) / 1000000} ms:")
    for b in report:
        line = []
        for i in range(8):
            line.append((b >> i) & 1)

        print(line)

    # Close connection
    s.send(CMD_CLOSE_CONN)


