import serial

# Initialize the serial connection to the ESP32
esp32 = serial.Serial(port='COM3', baudrate=115200, timeout=1)

print("Running violations detector")

while True:
    redLight, time, speed, distance = esp32.readline().decode("utf-8").split()

    if int(redLight) == 1 and float(distance) < 36: # Place traffic light ~40 cm away, allow car 4 cm room for error
        print(f"You are a traffic light violator! Your speed was {speed} cm/s.")
        break

# import cv2
# import pytesseract

# cap = cv2.VideoCapture(0)
    
# while True:
#     ret, frame = cap.read()

#     cv2.imshow('Video Stream', frame)
    
#     if cv2.waitKey(1) == ord('q'):
#         break

# cap.release()
# cv2.destroyAllWindows()