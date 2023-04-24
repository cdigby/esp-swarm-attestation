import os, sys, socket, secrets, hmac, hashlib

# Definitions from sa_shared.h
SIMPLE_KEY_SIZE         = 32
SIMPLE_HMAC_LEN         = 32

# Structure of a SIMPLE+ attest_req, from sa_simple_plus.h
SIMPLE_PLUS_ATTESTREQ_CV_LEN        = 4
SIMPLE_PLUS_ATTESTREQ_NONCE_LEN     = 32
SIMPLE_PLUS_ATTESTREQ_VSSLEN_LEN    = 2

# Command codes for prover TCP server
CMD_NODE_NAME           = 0x01.to_bytes(1, byteorder="little")
CMD_CLOSE_CONN          = 0x06.to_bytes(1, byteorder="little")
CMD_SIMPLE_PLUS_ATTEST  = 0x07.to_bytes(1, byteorder="little")

# Valid states of the fake memory region on the prover
VALID_STATES_RAW = [
    b'\x00' * 1024,
    b'\x01' * 1024,
    b'\x02' * 1024,
    b'\x03' * 1024,
]

# Get id of node we are connected to from CLI args
if len(sys.argv) != 2:
    print("Usage: python3 simple_verifier.py NODE_ID")
    sys.exit(1)

NODE_ID = None
try :
    NODE_ID = int(sys.argv[1])
except ValueError:
    print("NODE_ID must be an integer")
    sys.exit(1)

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
        with open("counters/simple_plus_cv.bin", "rb") as f:
            return int.from_bytes(f.read(), byteorder="little")
        
    except FileNotFoundError:
        return 0
    
def set_verifier_counter(value: int) -> None:
    with open("counters/simple_plus_cv.bin", "wb") as f:
        f.write(value.to_bytes(SIMPLE_PLUS_ATTESTREQ_CV_LEN, byteorder="little"))


# Functions to get and set k_col
# This also needs be stored in a file as it is updated on each run
# k_col is initialised from keys/k_col.bin and can be reset using reset_counters.py
def get_k_col() -> bytes:
    try:
        with open("counters/simple_plus_k_col.bin", "rb") as f:
            return f.read()
        
    except FileNotFoundError:
        with open("keys/k_col.bin", "rb") as f:
            return f.read()
        
def set_k_col(value: bytes) -> None:
    with open("counters/simple_plus_k_col.bin", "wb") as f:
        f.write(value)


# Connect to node
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    print(f"Connecting to node {NODE_ID} via {HOST}:{PORT}")
    s.connect((HOST, PORT))

    # Transmit name or our connection will be rejected
    node_name = b'VERIFIER\0'
    s.send(CMD_NODE_NAME + len(node_name).to_bytes(1, byteorder="little") + node_name)

    ### Algorithm as per Figure 4 of SIMPLE paper ###
    print("Executing SIMPLE+ Attestation Phase...")

    # Generate nonce
    nonce = secrets.token_bytes(SIMPLE_PLUS_ATTESTREQ_NONCE_LEN)

    # Increment counter and store new value
    cv = get_verifier_counter() + 1
    set_verifier_counter(cv)
    print(f"Counter: {cv}")

    # Compute valid software state hashes
    vss = bytes(0)
    for state in VALID_STATES_RAW:
        vss += hmac.digest(K_ATTEST, state, hashlib.sha256)

    # Compute HMAC and construct attest_req
    # Note that we need to add the length of VSS (2 bytes) before VSS
    data = cv.to_bytes(4, byteorder='little') + len(vss).to_bytes(2, byteorder='little') + vss + nonce
    h = hmac.digest(K_AUTH, data, hashlib.sha256)
    attest_req = data + h

    # Send attest_req
    print(f"Sending attestation request...")
    s.send(CMD_SIMPLE_PLUS_ATTEST + len(attest_req).to_bytes(2, byteorder='little') + attest_req)

    # Update k_col
    print("Updating k_col...")
    k_col = get_k_col()
    set_k_col(hashlib.sha256(k_col).digest())

    # Close connection
    s.send(CMD_CLOSE_CONN)

    print("Done!")



