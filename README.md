# Proyecto DHCP_Server

## Prerrequisitos

1. **Sistema o subsistema de Linux** (por ejemplo, Ubuntu, Debian, Fedora).
2. **Git** instalado en Ubuntu:
   ```bash
   sudo apt update
   sudo apt install git -y
   ```
3. **GCC** (compilador de C) instalado:
   ```bash
   sudo apt install gcc -y
   ```
4. **Python3** Instalado
   
## Clonar el Repositorio

1. Abre la terminal de Ubuntu y navega al directorio donde deseas clonar el repositorio.
2. Clona el repositorio ejecutando:
   ```bash
   git clone https://github.com/camazog1/DHCP_Server
   ```

## Compilar el Servidor DHCP en C

1. Navega al directorio donde se encuentra el archivo del servidor (`dhcp_server.c`):
   ```bash
   cd DHCP_Server
   ```
2. Compila el programa en C usando `gcc`:
   ```bash
   gcc -o dhcp_server dhcp_server.c
   ```
3. Inicia el servidor ejecutando el binario:
   ```bash
   sudo ./dhcp_server
   ```

## Ejecutar el Cliente DHCP en Python

1. Abre una nueva terminal. (deja el servidor ejecutándose en la primera).
2. Navega al directorio del proyecto:
   ```bash
   cd DHCP_Server
   ```
3. Ejecuta el cliente DHCP en Python:
   ```bash
   python3 dhcp_client.py
   ```
