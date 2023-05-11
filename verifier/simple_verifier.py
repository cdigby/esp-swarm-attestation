import os, sys, socket, secrets, hmac, hashlib, time

# Definitions from sa_shared.h
SIMPLE_KEY_SIZE         = 32
SIMPLE_HMAC_LEN         = 32

# Structure of a SIMPLE msg, from sa_simple.h
SIMPLE_MSG_CV_LEN           = 4
SIMPLE_MSG_VS_LEN           = 32
SIMPLE_MSG_NONCE_LEN        = 32
SIMPLE_MSG_HMAC_LEN         = SIMPLE_HMAC_LEN
SIMPLE_MSG_LEN              = (SIMPLE_MSG_CV_LEN + SIMPLE_MSG_VS_LEN + SIMPLE_MSG_NONCE_LEN + SIMPLE_MSG_HMAC_LEN)

# Structure of SIMPLE report, from sa_simple.h
SIMPLE_REPORT_VALUE_LEN     = 1
SIMPLE_REPORT_HMAC_LEN      = 32
SIMPLE_REPORT_LEN           = (SIMPLE_REPORT_VALUE_LEN + SIMPLE_REPORT_HMAC_LEN)

SIMPLE_REPORT_VALUE_OFFSET  = 0
SIMPLE_REPORT_HMAC_OFFSET   = (SIMPLE_REPORT_VALUE_OFFSET + SIMPLE_REPORT_VALUE_LEN)

# Command codes for prover TCP server
CMD_NODE_NAME     = 0x01.to_bytes(1, byteorder="little")
CMD_SIMPLE_ATTEST = 0x05.to_bytes(1, byteorder="little")
CMD_CLOSE_CONN    = 0x06.to_bytes(1, byteorder="little")

# Get NODE_ID and VALID_STATE from CLI args
if len(sys.argv) != 3:
    print("Usage: python3 simple_verifier.py NODE_ID VALID_STATE")
    sys.exit(1)

NODE_ID = None
try :
    NODE_ID = int(sys.argv[1])
except Exception:
    print("NODE_ID must be an integer")
    sys.exit(1)

VALID_STATE = None
try :
    VALID_STATE = bytes.fromhex(sys.argv[2])
except Exception:
    print("VALID_STATE must be a hex byte e.g. 01 or ab")
    sys.exit(1)

if len(VALID_STATE) != 1:
    print("VALID_STATE must be a single byte")
    sys.exit(1)

# Fake memory region considered the valid state for the prover
# Prover considered valid if the hash of FAKE_MEMORY_REGION is equal to the hash of FAKE_MEMORY_REGION on the prover
FAKE_MEMORY_REGION = VALID_STATE * (64 * 1024) 

# Generate host IP
HOST = f"192.168.{NODE_ID}.1"
PORT = 3333

# Load HMAC keys
K_AUTH = None
K_ATTEST = None
with open("keys/k_auth.bin", "rb") as f:
    K_AUTH = f.read()

with open("keys/k_attest.bin", "rb") as f:
    K_ATTEST = f.read()

# Make counters directory
os.makedirs("counters", exist_ok=True)

# Functions to get and set verifier counter
# We store it in a file as it needs to persist between runs
# Use reset_counters.py to reset all counters
def get_verifier_counter() -> int:
    try:
        with open("counters/simple_cv.bin", "rb") as f:
            return int.from_bytes(f.read(), byteorder="little")
        
    except FileNotFoundError:
        return 0
    
def set_verifier_counter(value: int) -> None:
    with open("counters/simple_cv.bin", "wb") as f:
        f.write(value.to_bytes(SIMPLE_MSG_CV_LEN, byteorder="little"))

# Connect to node
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    print(f"Connecting to node {NODE_ID} via {HOST}:{PORT}")
    s.connect((HOST, PORT))

    # Transmit name or our connection will be rejected
    node_name = b'VERIFIER\0'
    s.send(CMD_NODE_NAME + len(node_name).to_bytes(1, byteorder="little") + node_name)

    ### Algorithm as per Figure 2 of SIMPLE paper ###
    print("Executing SIMPLE...")

    # Generate nonce
    nonce = secrets.token_bytes(SIMPLE_MSG_NONCE_LEN)

    # Increment counter and store new value
    cv = get_verifier_counter() + 1
    set_verifier_counter(cv)
    print(f"Counter: {cv}")

    # Compute valid software state hash
    vs = hmac.digest(K_ATTEST, FAKE_MEMORY_REGION, hashlib.sha256)

    # Compute HMAC and construct message
    msg_data = cv.to_bytes(4, byteorder='little') + vs + nonce
    h = hmac.digest(K_AUTH, msg_data, hashlib.sha256)
    msg = msg_data + h

    # Send message to node
    print(f"Sending attestation request...")
    start_time = time.perf_counter_ns()
    s.send(CMD_SIMPLE_ATTEST + len(msg).to_bytes(2, byteorder='little') + msg)

    # Get report
    print("Waiting for response...")
    report = s.recv(SIMPLE_REPORT_LEN, socket.MSG_WAITALL)
    stop_time = time.perf_counter_ns()
    print(f"Received report in {(stop_time - start_time) / 1000000} ms")

    # Verify HMAC of report
    report_value = report[SIMPLE_REPORT_VALUE_OFFSET : SIMPLE_REPORT_VALUE_OFFSET + SIMPLE_REPORT_VALUE_LEN]
    report_hmac = report[SIMPLE_REPORT_HMAC_OFFSET : SIMPLE_REPORT_HMAC_OFFSET + SIMPLE_REPORT_HMAC_LEN]
    local_report_hmac = hmac.digest(K_AUTH, report_value + cv.to_bytes(4, byteorder='little') + nonce, hashlib.sha256)
    if hmac.compare_digest(report_hmac, local_report_hmac):
        print("HMAC verified")

        # Print result
        v = int.from_bytes(report_value, byteorder="little")
        if v == 1:
            print(f"Report value is {v}: prover is in a valid state")

        else:
            print(f"Report value is {v}: prover is not in a valid state")

    else:
        print("HMAC invalid")

    
    # Close connection
    s.send(CMD_CLOSE_CONN)