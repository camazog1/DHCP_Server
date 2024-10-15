import socket
import re

# Definir la IP y el puerto del servidor
SERVER_IP = "255.255.255.255"
SERVER_PORT = 67

def parse_response(response):
    # Expresiones regulares para buscar cada componente
    ip_pattern = r'IP:\s*(\S+)'
    mask_pattern = r'Mascara:\s*(\S+)'
    dns_pattern = r'DNS:\s*(\S+)'
    gateway_pattern = r'Gateway:\s*(\S+)'

    # Buscar coincidencias en la respuesta usando expresiones regulares
    ip_match = re.search(ip_pattern, response)
    mask_match = re.search(mask_pattern, response)
    dns_match = re.search(dns_pattern, response)
    gateway_match = re.search(gateway_pattern, response)

    # Asignar a variables
    IP = ip_match.group(1) if ip_match else None
    MASK = mask_match.group(1) if mask_match else None
    DNS = dns_match.group(1) if dns_match else None
    GATEWAY = gateway_match.group(1) if gateway_match else None

    return IP, MASK, DNS, GATEWAY

def dchp_release():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    message = "DHCPRELEASE: " + str(IP)
    client_socket.sendto(message.encode(), (GATEWAY, SERVER_PORT)) 
    client_socket.close()

def dhcp_request(ip, mask, dns, gateway):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 
    message = "DHCPREQUEST: " + ip
    
    # Enviar el mensaje DHCPREQUEST a la dirección GATEWAY en lugar de broadcast
    client_socket.sendto(message.encode(), (gateway, SERVER_PORT)) 
    
    response, _ = client_socket.recvfrom(1024) 
    response_decoded = response.decode()
    print("Respuesta del servidor:", response_decoded)
    
    if response_decoded.find("OK") != -1:
        print("IP:", ip)
        print("Máscara:", mask)
        print("DNS:", dns)
        print("Gateway:", gateway)
    else:
        print("Error en la solicitud DHCP. Configurando según APIPA...")
        print("IP: 169.254.0.1")
        print("Máscara: 255.255.0.0")
        print("DNS: 8.8.8.8")
        print("Gateway: ''")  
        

def dhcp_discover():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)  # Habilitar el broadcast
    message = "DHCPDISCOVER"
    
    try:
        client_socket.sendto(message.encode(), (SERVER_IP, SERVER_PORT))
        response, _ = client_socket.recvfrom(1024)
        response_decoded = response.decode()

        if response_decoded != "No hay IPs disponibles.":
            ip, mask, dns, gateway = parse_response(response_decoded)
            dhcp_request(ip, mask, dns, gateway)
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
    while True:
        print("1. DISCOVER\n2. RELEASE\n3. IPCONFIG")
        option = int(input("Seleccione: "))
        if option == 1:
            if IP is None:
                IP, MASK, DNS, GATEWAY = dhcp_discover()
            else:
                print("Ya existe IP asignada")
                print("IP:", IP)
                print("Máscara:", MASK)
                print("DNS:", DNS)
                print("Gateway:", GATEWAY)
        elif option == 2:
            if IP is not None:
                dchp_release()
                IP = None
                MASK = None
                DNS = None
                GATEWAY = None
        elif option == 3:
            print("IP:", IP)
            print("Máscara:", MASK)
            print("DNS:", DNS)
            print("Gateway:", GATEWAY)
        else:
            if IP is not None:
                dchp_release()
            break
