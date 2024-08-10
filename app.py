import dotenv
dotenv.load_dotenv()

import pygame
import time
import subprocess
import os
import aiohttp
import asyncio
from memory_profiler import profile

async def make_post_request(url, data):
    async with aiohttp.ClientSession() as session:
        async with session.post(url, json=data) as response:
            if response.status == 200:
                result = await response.json()
                return result
            else:
                return {"error": response.status, "message": await response.text()}

def run_steam_app(appid):
    try:
        # Comando para ejecutar Steam con el AppID proporcionado
        comando = f'steam -applaunch {appid}'
        
        # Ejecutar el comando en el sistema operativo
        subprocess.run(comando, shell=True)
    except Exception as e:
        print(f"Error al ejecutar el juego: {e}")

@profile
def main():
    # Definir el botón que quieres monitorear (de acuerdo a la documentación de tu mando)
    BOTON = 0  # Esto podría ser diferente dependiendo de tu mando

    # Tiempo de espera máximo para contar las pulsaciones (en segundos)
    TIEMPO_MAXIMO = 0.3
    
    # Servidor de SplitPay
    SPLITPAY_SERVER = os.getenv("SPLITPAY_SERVER")
    
    MONEY_REQUEST = []

    # Configurar el contador y el temporizador
    contador = 0
    tiempo_inicio = None
    mando = False

    # Configurar la entrada del joystick
    while not mando:
        try:
            pygame.joystick.init()
            joystick = pygame.joystick.Joystick(0)
            joystick.init()
            mando = True
        except:
            pygame.joystick.quit()
            os.system("cls")
            print("Conecta un mando !")
            time.sleep(1)

    pygame.init()
    
    os.system("cls")
    print("SplitPay: Esperando a Insertar Monedas")
    ejecutando = True
    payload = {}
    
    async def event_loop():
        nonlocal contador, tiempo_inicio, ejecutando, payload

        while ejecutando:
            for event in pygame.event.get():
                if event.type == pygame.JOYDEVICEREMOVED:
                    os.system("cls")
                    joystick.quit()
                    print("Conecta un mando")
                if event.type == pygame.JOYDEVICEADDED:
                    os.system("cls")
                    joystick.init()
                    print("SplitPay: Esperando a Insertar Monedas")

                if event.type == pygame.JOYBUTTONDOWN:
                    if event.button == 2:
                        os.system("cls")
                        print(MONEY_REQUEST)
                    if event.button == 1:
                        ejecutando = False
                    if event.button == BOTON:
                        contador += 1
                        if tiempo_inicio is None:
                            tiempo_inicio = time.time()
                if event.type == pygame.KEYDOWN:
                    print("xd")
                    ejecutando = False

            if tiempo_inicio is not None:
                os.system("cls")
                print(f"TIEMPO: {tiempo_inicio is not None and time.time() - tiempo_inicio} CONTADOR: {contador}")

            if tiempo_inicio is not None and time.time() - tiempo_inicio >= TIEMPO_MAXIMO:
                if contador == 1:
                    print("$0.25")
                    payload = {"value": "0.25"}
                elif contador == 2:
                    print("$1")
                    payload = {"value": "1.00"}
                elif contador == 3:
                    print("$0.05")
                    payload = {"value": "0.05"}
                elif contador == 5:
                    print("$0.1")
                    payload = {"value": "0.10"}
                
                asyncio.create_task(make_post_request(f"{SPLITPAY_SERVER}/deposit", payload))

                contador = 0
                tiempo_inicio = None

            if event.type == pygame.QUIT:
                ejecutando = False

            await asyncio.sleep(0.01)  # Add a small sleep to avoid a busy loop

        pygame.quit()

    asyncio.run(event_loop())

if __name__ == "__main__":
    main()
