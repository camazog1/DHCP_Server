import socket
import re
import time
import threading

# Definir la IP y el puerto del servidor
SERVER_IP = "255.255.255.255"
SERVER_PORT = 67
CLIENT_PORT = 68
lease_expiration_time = None
show_info = threading.Event()
is_released = False  # Nueva variable de control

def parse_response(response):
    # Expresiones regulares para buscar cada componente
    ip_pattern = r'IP:\s*(\S+)'
    mask_pattern = r'Mascara:\s*(\S+)'
    dns_pattern = r'DNS:\s*(\S+)'
    gateway_pattern = r'Gateway:\s*(\S+)'
    lease_pattern = r'Tiempo de arrendamiento:\s*(\d+)'

    # Buscar coincidencias en la respuesta usando expresiones regulares
    ip_match = re.search(ip_pattern, response)
    mask_match = re.search(mask_pattern, response)
    dns_match = re.search(dns_pattern, response)
    gateway_match = re.search(gateway_pattern, response)
    lease_match = re.search(lease_pattern, response)

    # Asignar a variables
    IP = ip_match.group(1) if ip_match else None
    MASK = mask_match.group(1) if mask_match else None
    DNS = dns_match.group(1) if dns_match else None
    GATEWAY = gateway_match.group(1) if gateway_match else None
    LEASE_TIME = int(lease_match.group(1)) if lease_match else None

    threading.Thread(target=lease_timer, daemon=True).start()

    return IP, MASK, DNS, GATEWAY, LEASE_TIME

def dchp_release():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    message = "DHCPRELEASE: " + str(IP)
    client_socket.sendto(message.encode(), (GATEWAY, SERVER_PORT)) 
    client_socket.close()

def lease_timer():
    global lease_expiration_time, IP, MASK, DNS, GATEWAY, LEASE_TIME, is_released
    while True:
        if lease_expiration_time:
            remaining_time = lease_expiration_time - time.time()
            if remaining_time <= 0:
                if not is_released: 
                    print("El tiempo de arrendamiento ha expirado.")
                # Poner todo en None
                IP, MASK, DNS, GATEWAY, LEASE_TIME = None, None, None, None, None
                lease_expiration_time = None
                show_info.clear()
                break
        time.sleep(1) 


def dhcp_request(ip, mask, dns, gateway, lease_time):
    global lease_expiration_time
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 
    message = f"DHCPREQUEST: {ip}"
    
    client_socket.sendto(message.encode(), (gateway, SERVER_PORT)) 
    
    response, _ = client_socket.recvfrom(1024) 
    response_decoded = response.decode()
    print("Respuesta del servidor:", response_decoded)
    
    if "OK" in response_decoded:
        print(f"IP: {ip}\nMáscara: {mask}\nDNS: {dns}\nGateway: {gateway}\nTiempo de arrendamiento: {lease_time} segundos")
        lease_expiration_time = time.time() + lease_time
    else:
        print("Error en la solicitud DHCP. Configurando según APIPA...")
        print("IP: 169.254.0.1")
        print("Máscara: 255.255.0.0")
        print("DNS: 8.8.8.8")
        print("Gateway: ''")
        lease_expiration_time = None

def dhcp_discover():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    message = "DHCPDISCOVER"
    
    try:
        client_socket.sendto(message.encode(), (SERVER_IP, SERVER_PORT))
        response, _ = client_socket.recvfrom(1024)
        response_decoded = response.decode()

        if response_decoded != "No hay IPs disponibles.":
            ip, mask, dns, gateway, lease_time = parse_response(response_decoded)
            dhcp_request(ip, mask, dns, gateway, lease_time)
        else:
            print("El servidor no tiene IP's disponibles")
            ip, mask, dns, gateway = None, None, None, None
    except Exception as e:
        print("Error:", e)
    finally:
        client_socket.close() 
    
    return ip, mask, dns, gateway

if __name__ == "__main__":
    IP = None
    MASK = None
    DNS = None
    GATEWAY = None
    LEASE_TIME = None

    while True:
        print("1. DISCOVER\n2. RELEASE\n3. IPCONFIG")
        option = int(input("Seleccione: "))
        
        if option == 1:
            if IP is None:
                # Iniciar el hilo para escuchar expiraciones
                threading.Thread(target=lease_timer, daemon=True).start()
                IP, MASK, DNS, GATEWAY = dhcp_discover()
                show_info.set() 
                is_released = False
            else:
                print("Ya existe IP asignada")
                print(f"IP: {IP}\nMáscara: {MASK}\nDNS: {DNS}\nGateway: {GATEWAY}")
                
        elif option == 2:
            if IP is not None:
                dchp_release()
                IP = None
                MASK = None
                DNS = None
                GATEWAY = None
                show_info.clear()  
                is_released = True
                
        elif option == 3:
            if show_info.is_set():  # Verificar si se permite mostrar información
                print(f"IP: {IP}\nMáscara: {MASK}\nDNS: {DNS}\nGateway: {GATEWAY}")
            else:
                print("No hay información para mostrar.")
                
        else:
            if IP is not None:
                dchp_release()
            break