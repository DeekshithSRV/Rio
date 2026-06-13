
import paho.mqtt.client as mqtt
import time
import threading

# ================= MQTT =================
BROKER = "test.mosquitto.org"
PORT = 1883

# ================= GLOBAL VARIABLES =================
device_id = None
device_online = False

statusTopic = ""
ackTopic = ""
lightTopic = ""
fanTopic = ""
pumpTopic = ""

# ================= TANK SIMULATION =================
tank_level = 0
pump_running = False


# ====================================================
# MQTT CONNECTED
# ====================================================
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker")
        client.subscribe("home/discovery")
        print("Waiting for ESP32...")
    else:
        print(f"MQTT Connection Failed : {rc}")


# ====================================================
# MQTT MESSAGES
# ====================================================
def on_message(client, userdata, msg):
    global device_id
    global device_online
    global statusTopic
    global ackTopic
    global lightTopic
    global fanTopic
    global pumpTopic

    topic = msg.topic
    payload = msg.payload.decode().strip()

    # ---------------- DISCOVERY ----------------
    if topic == "home/discovery":

        if device_id is None:
            device_id = payload

            statusTopic = f"home/{device_id}/device/status"
            ackTopic = f"home/{device_id}/ack"

            lightTopic = f"home/{device_id}/actions/light"
            fanTopic = f"home/{device_id}/actions/fan"
            pumpTopic = f"home/{device_id}/actions/pump"

            client.subscribe(statusTopic)
            client.subscribe(ackTopic)

            print(f"\nESP32 Found : {device_id}")

    # ---------------- ONLINE / OFFLINE ----------------
    elif topic == statusTopic:

        if payload == "ONLINE":
            if not device_online:
                device_online = True
                print(f"\nESP32 {device_id} : ONLINE")

        elif payload == "OFFLINE":
            if device_online:
                device_online = False
                print(f"\nESP32 {device_id} : OFFLINE")

    # ---------------- ACK ----------------
    elif topic == ackTopic:
        print(f"\nACK : {payload}")


# ====================================================
# TANK SIMULATION
# ====================================================
def tank_simulation():
    global tank_level
    global pump_running

    while True:

        if pump_running:

            if tank_level < 100:

                tank_level += 5

                if tank_level > 100:
                    tank_level = 100

                print(f"\nTank Level : {tank_level}%")

                if tank_level >= 100:

                    tank_level = 100
                    pump_running = False

                    print("\nTANK FULL")
                    print("PUMP AUTO OFF")

                    if device_online:
                        client.publish(
                            pumpTopic,
                            "OFF"
                        )

        time.sleep(1)


# ====================================================
# MQTT CLIENT
# ====================================================
client = mqtt.Client()

client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)
client.loop_start()

# Wait for ESP32 Discovery
while device_id is None:
    time.sleep(1)

# Start Tank Simulation
threading.Thread(
    target=tank_simulation,
    daemon=True
).start()


# ====================================================
# MENU LOOP
# ====================================================
while True:

    print("\n===== MENU =====")
    print("1. FAN")
    print("2. LIGHT")
    print("3. PUMP")
    print("4. STATUS")
    print("5. ALL OFF")
    print("6. EMPTY TANK")
    print("7. FILL TANK TO 100%")
    print("q. EXIT")

    choice = input("\nEnter Choice : ").strip()

    # =================================================
    # EXIT
    # =================================================
    if choice.lower() == "q":
        break

    # =================================================
    # FAN
    # =================================================
    elif choice == "1":

        if not device_online:
            print("ESP32 OFFLINE")
            continue

        print("FAN Selected")

        state = input(
            "Enter ON or OFF : "
        ).strip().upper()

        if state in ["ON", "OFF"]:

            client.publish(
                fanTopic,
                state
            )

            print(f"FAN {state} Sent")

        else:
            print("Invalid Command")

    # =================================================
    # LIGHT
    # =================================================
    elif choice == "2":

        if not device_online:
            print("ESP32 OFFLINE")
            continue

        print("LIGHT Selected")

        state = input(
            "Enter ON or OFF : "
        ).strip().upper()

        if state in ["ON", "OFF"]:

            client.publish(
                lightTopic,
                state
            )

            print(f"LIGHT {state} Sent")

        else:
            print("Invalid Command")

    # =================================================
    # PUMP
    # =================================================
    elif choice == "3":

        if not device_online:
            print("ESP32 OFFLINE")
            continue

        print("PUMP Selected")

        state = input(
            "Enter ON or OFF : "
        ).strip().upper()

        if state not in ["ON", "OFF"]:
            print("Invalid Command")
            continue

        # ---------- PUMP ON ----------
        if state == "ON":

            if tank_level >= 100:

                pump_running = False

                if device_online:
                    client.publish(
                        pumpTopic,
                        "OFF"
                    )

                print("\nTANK IS FULL")
                print("PUMP IS OFF")

            else:

                client.publish(
                    pumpTopic,
                    "ON"
                )

                pump_running = True

                print("PUMP ON Sent")

        # ---------- PUMP OFF ----------
        else:

            client.publish(
                pumpTopic,
                "OFF"
            )

            pump_running = False

            print("PUMP OFF Sent")

    # =================================================
    # STATUS
    # =================================================
    elif choice == "4":

        print("\n===== ESP32 STATUS =====")

        if device_id:
            print(f"Device ID : {device_id}")

        print(
            "ESP32 : "
            + (
                "ONLINE"
                if device_online
                else "OFFLINE"
            )
        )

        print(f"Tank Level : {tank_level}%")

        print(
            "Pump : "
            + (
                "ON"
                if pump_running
                else "OFF"
            )
        )

    # =================================================
    # ALL OFF
    # =================================================
    elif choice == "5":

        if not device_online:
            print("ESP32 OFFLINE")
            continue

        client.publish(
            fanTopic,
            "OFF"
        )

        client.publish(
            lightTopic,
            "OFF"
        )

        client.publish(
            pumpTopic,
            "OFF"
        )

        pump_running = False

        print("ALL DEVICES OFF")

    # =================================================
    # EMPTY TANK
    # =================================================
    elif choice == "6":

        tank_level = 0
        pump_running = False

        if device_online:
            client.publish(
                pumpTopic,
                "OFF"
            )

        print("TANK EMPTIED")
        print("Tank Level : 0%")

    # =================================================
    # FILL TANK TO 100%
    # =================================================
    elif choice == "7":

        tank_level = 100
        pump_running = False

        if device_online:
            client.publish(
                pumpTopic,
                "OFF"
            )

        print("TANK FILLED TO 100%")
        print("Tank Level : 100%")

    else:
        print("Invalid Choice")


# ====================================================
# CLEANUP
# ====================================================
client.loop_stop()
client.disconnect()

