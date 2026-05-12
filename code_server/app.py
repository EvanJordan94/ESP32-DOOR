from flask import Flask, render_template, jsonify, request
import cv2, numpy as np, requests, os, json
import time
from PIL import Image
import paho.mqtt.client as mqtt
from datetime import datetime

# ==== NHẬN DIỆN KHUÔN MẶT BẰNG OpenCV ====
recognizer = cv2.face.LBPHFaceRecognizer_create()
recognizer.read('trainer/trainer.yml')  # Đã huấn luyện bằng 02_face_training.py
faceCascade = cv2.CascadeClassifier("haarcascade_frontalface_default.xml")
  
# Đọc danh sách tên từ file
# def load_names():
    # with open("names.txt", "r", encoding="utf-8") as f:
        # return [line.strip() for line in f]
# names = load_names()

# Danh sách tên tương ứng với ID đã huấn luyện 
names = ['None', 'Me', 'Quyet', 'Duong', 'Thanh', 'Giang', 'Kha Banh', "Elon Musk"]

def recognize_cv2(frame):
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    faces = faceCascade.detectMultiScale(
        gray,
        scaleFactor=1.2,
        minNeighbors=5,
        minSize=(int(0.1 * frame.shape[1]), int(0.1 * frame.shape[0]))
    )

    for (x, y, w, h) in faces:
        id_, confidence = recognizer.predict(gray[y:y+h, x:x+w])
        if confidence < 100:
            name = names[id_]
            conf = round(100 - confidence) / 100.0
        else:
            name = "unknown"
            conf = round(100 - confidence) / 100.0
        return name, conf

    return "unknown", 0.0

# ==== CẤU HÌNH ====
ESP32_CAM = "http://192.168.248.44/cam-hi.jpg"

MQTT_TOPIC_FACE = "esp32cam/face"
MQTT_TOPIC_PASS = "esp32/data/password"
MQTT_TOPIC_RFID = "esp32/data/rfid_list"
MQTT_TOPIC_FP = "esp32/data/finger_ids"
MQTT_BROKER = "0959e8f58b7b4eff970d0713cc9c4bfd.s1.eu.hivemq.cloud"
MQTT_USER = "EvanJordan"
MQTT_PASS = "Thanh94@"

if not os.path.exists("face_metadata.json"):
    with open("face_metadata.json", "w") as f:
        json.dump([], f)


# ==== GLOBAL ACCESS DATA ====
access_data = {}
metadata_path = 'face_metadata.json'
current_face_action = "none"

# ==== LOAD access_data.json nếu có ====
if os.path.exists("access_data.json"):
    with open("access_data.json", "r", encoding="utf-8") as f:
        try:
            access_data = json.load(f)
            print("📂 Đã tải access_data.json:", access_data)
        except Exception as e:
            print("❌ Lỗi đọc access_data.json:", e)

# ==== MQTT ====
mqttc = mqtt.Client()
mqttc.tls_set()
mqttc.username_pw_set(MQTT_USER, MQTT_PASS)

def on_connect(client, userdata, flags, rc):
    print("✅ Kết nối MQTT thành công" if rc == 0 else f"❌ Lỗi kết nối MQTT: {rc}")
    client.subscribe(MQTT_TOPIC_PASS)
    client.subscribe(MQTT_TOPIC_RFID)
    client.subscribe(MQTT_TOPIC_FP) 
    client.subscribe("esp32/face/cmd")

def save_access_data():
    try:
        with open("access_data.json", "w", encoding="utf-8") as f:
            json.dump(access_data, f, ensure_ascii=False, indent=2)
        print("💾 Đã lưu access_data.json:", access_data)
    except Exception as e:
        print("❌ Không thể ghi access_data.json:", e)

def on_message(client, userdata, msg): 
    global access_data
    topic = msg.topic
    payload = msg.payload.decode('utf-8')

    print(f"📩 Nhận từ {topic}: {payload}")

    try:
        if not isinstance(access_data, dict):
            access_data = {}

        if topic == MQTT_TOPIC_PASS:
            access_data["password"] = payload
            print("🔑 Mật khẩu cập nhật:", payload)
            save_access_data()

        elif topic == MQTT_TOPIC_RFID:
            rfid_list = [uid.strip().upper() for uid in payload.split(',') if uid.strip()]
            access_data["rfid_list"] = rfid_list
            print("📇 Danh sách RFID:", rfid_list)
            save_access_data()

        elif topic == MQTT_TOPIC_FP:
            fp_list = [int(fpid.strip()) for fpid in payload.split(',') if fpid.strip().isdigit()]
            access_data["fingerprint_ids"] = fp_list
            print("🧬 Danh sách ID vân tay:", fp_list)
            save_access_data()
        elif topic == "esp32/face/cmd":
            global current_face_action
            payload = payload.strip()
            if payload == "add":
                current_face_action = "add"
                print("📥 Lệnh từ ESP32: THÊM khuôn mặt")
            elif payload == "delete":
                current_face_action = "delete"
                print("📥 Lệnh từ ESP32: XOÁ khuôn mặt")
            else:
                current_face_action = "none"
                print("📥 Lệnh từ ESP32: KHÔNG rõ hành động")
    
        else:
            print("⚠️ Không xử lý topic này:", topic)

    except Exception as e:
        print("❌ Lỗi xử lý MQTT:", e)

mqttc.on_connect = on_connect
mqttc.on_message = on_message
mqttc.connect(MQTT_BROKER, 8883)
mqttc.loop_start()


# ==== LẤY FRAME TỪ CAMERA ====
# def fetch_frame():
#     cap = cv2.VideoCapture(0)  # Webcam mặc định
#     if not cap.isOpened():
#         print("❌ Không thể mở webcam")
#         return None
#     ret, frame = cap.read()
#     cap.release()
#     return frame if ret else None
# ==== LẤY FRAME TỪ ESP32 ====
def fetch_frame():
    try:
        r = requests.get(ESP32_CAM, timeout=3)
        arr = np.frombuffer(r.content, np.uint8)
        return cv2.imdecode(arr, cv2.IMREAD_COLOR)
    except:
        return None
# ==== FLASK APP ====
app = Flask(__name__)

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/predict_tm", methods=["POST"])
def predict_tm() :
    frame = fetch_frame()
    if frame is None:
        return jsonify({"error": "Không lấy được ảnh từ webcam"}), 500

    label, conf = recognize_cv2(frame)
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    print(f"[AI] Nhãn: {label} | Tự tin: {conf:.2f}")

    image_path = None
    if label.lower() in ["None", "unknown"] or conf < 0.2:
        result = mqttc.publish(MQTT_TOPIC_FACE, "door:0")
        if result.rc != 0:
            print("❌ Gửi MQTT thất bại:", result.rc)
        else:
            print("🚫 Gửi MQTT: door:0")
    else:
        payload = f"door:1,name:{label}"
        result = mqttc.publish(MQTT_TOPIC_FACE, payload)
        if result.rc != 0:
            print("❌ Gửi MQTT thất bại:", result.rc)
        else:
            print(f"✅ Gửi MQTT: {payload}")

        os.makedirs("logs", exist_ok=True)
        image_path = f"logs/{label}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.jpg"
        cv2.imwrite(image_path, frame)
        print(f"💾 Ảnh đã lưu: {image_path}")

# ==== GHI DỮ LIỆU VÀO data.json ====
    data = {
        "label": label,
        "confidence": round(conf, 3),
        "timestamp": timestamp,
        "image_path": image_path,
        "access_data": access_data
    }

    try:
        with open("data.json", "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
        print("📝 Đã ghi vào data.json:", os.path.abspath("data.json"))
    except Exception as e:
        print("❌ Lỗi ghi file data.json:", e)

    return jsonify(data)


@app.route('/face_action')
def face_action():
    global current_face_action
    return jsonify({"action": current_face_action})


@app.route("/add_face", methods=["POST"])
def add_face():

    name = request.form.get('name')  # lấy tên từ form web

    if not name:
        return jsonify({"error": "Thiếu tên người dùng"}), 400

    # Load metadata nếu đã tồn tại
    metadata_path = "face_metadata.json"
    if os.path.exists(metadata_path):
        with open(metadata_path, "r", encoding="utf-8") as f:
            metadata = json.load(f)
    else:
        metadata = {}

    # Kiểm tra tên đã tồn tại chưa (ở ID khác)
    if name in metadata.values():
        mqttc.publish("esp32/face/status", "name_exists")
        return jsonify({"error": f"Tên '{name}' đã tồn tại với ID khác"}), 400
    
    # Tạo ID mới bằng cách đếm số dòng trong names.txt
    with open("names.txt", "r", encoding="utf-8") as f:
        current_names = f.readlines()
    new_id = len(current_names)
     # Thêm tên mới vào names.txt
    with open("names.txt", "a", encoding="utf-8") as f:
        f.write(name + "\n")

    # Gửi trạng thái bắt đầu chụp ảnh
    mqttc.publish("esp32/face/status", "start_capture")

    # Ghi 100 ảnh từ esp32cam_url
    for i in range(1, 101):
        try:
            resp = requests.get(ESP32_CAM, timeout=3)
            if resp.status_code == 200:
                filename = f"dataset/User.{new_id}.{i}.jpg"
                with open(filename, "wb") as f:
                    f.write(resp.content)
                print(f"Saved {filename}")
        except:
            print("Lỗi khi lấy ảnh")
        time.sleep(0.2)
     # Gửi trạng thái bắt đầu đã chụp ảnh
    mqttc.publish("esp32/face/status", "done_capture")

    # Lưu tên vào metadata
    metadata[str(new_id)] = name
    with open(metadata_path, "w", encoding="utf-8") as f:
        json.dump(metadata, f, ensure_ascii=False, indent=2)

    # Huấn luyện lại model
    mqttc.publish("esp32/face/status", "start_training")
    retrain_model()
    mqttc.publish("esp32/face/status", "done_training")

    return jsonify({"message": f"Đã thêm ID {new_id} - Tên {name}"}), 200


@app.route("/delete_face/<int:face_id>", methods=["DELETE"])
def delete_face(face_id):
    dataset_dir = "dataset"
    deleted_files = 0

    for fname in os.listdir(dataset_dir):
        if fname.startswith(f"User.{face_id}."):
            os.remove(os.path.join(dataset_dir, fname))
            deleted_files += 1

    if deleted_files == 0:
        return jsonify({"error": f"Không tìm thấy khuôn mặt với ID {face_id}."}), 404

    retrain_model()
    return jsonify({"message": f"🗑️ Đã xóa khuôn mặt ID {face_id} và huấn luyện lại."})

def retrain_model():
    global recognizer
    print("🔄 Bắt đầu huấn luyện lại...")
    from PIL import Image

    def getImagesAndLabels(path):
        imagePaths = [os.path.join(path,f) for f in os.listdir(path)]     
        faceSamples=[]
        ids = []
        for imagePath in imagePaths:
            PIL_img = Image.open(imagePath).convert('L') 
            img_numpy = np.array(PIL_img,'uint8')
            id = int(os.path.split(imagePath)[-1].split(".")[1])
            faces = faceCascade.detectMultiScale(img_numpy)
            for (x,y,w,h) in faces:
                faceSamples.append(img_numpy[y:y+h,x:x+w])
                ids.append(id)
        return faceSamples, ids

    faces, ids = getImagesAndLabels("dataset")
    if len(faces) == 0:
        print("❌ Không có dữ liệu khuôn mặt để huấn luyện.")
        return

    recognizer.train(faces, np.array(ids))
    os.makedirs("trainer", exist_ok=True)
    recognizer.write("trainer/trainer.yml")
    print("✅ Đã huấn luyện lại và lưu trainer.yml")
    recognizer.read("trainer/trainer.yml")
    print("✅ Đã load lại model vào RAM sau khi train")


@app.route("/access_data")
def get_access_data():
    return jsonify(access_data)

@app.route('/data.json')
def serve_data_json():
    try:
        with open('data.json', 'r', encoding='utf-8') as f:
            data = json.load(f)
        return jsonify(data)
    except FileNotFoundError:
        return jsonify({"error": "data.json not found"}), 404
    except json.JSONDecodeError:
        return jsonify({"error": "data.json is invalid JSON"}), 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
