import socket
import struct

# Define the structure format
struct_format = '2s 2i'  # 2 bytes string + 2 integers

# Pack the data into bytes
data = struct.pack(struct_format, b'AB', 1234, 5678)

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# Set the destination address and port
address = ('localhost', 9090)

while (True):
    
    command = input()

    # Send the data
    sock.sendto(command.encode(), address)

    if command == "exit":
        break

# Close the socket
sock.close()