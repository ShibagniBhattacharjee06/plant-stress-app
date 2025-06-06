import os
import json
from datetime import datetime
from flask import Flask, render_template, request, redirect, url_for
from werkzeug.utils import secure_filename
from tensorflow.keras.preprocessing.image import load_img, img_to_array
import numpy as np
import tensorflow as tf
import smtplib
from email.message import EmailMessage

app = Flask(__name__)

UPLOAD_FOLDER = 'static/uploads'
HISTORY_FILE = 'upload_history.json'
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
ALLOWED_EXTENSIONS = {'png', 'jpg', 'jpeg', 'gif'}
import os
import requests

MODEL_PATH = 'model/my_model.keras'
MODEL_URL = 'https://github.com/ShibagniBhattacharjee06/plant-stress-app/releases/download/model-v1/my_model.keras'

def download_model():
    if not os.path.exists('model'):
        os.makedirs('model')

    if not os.path.exists(MODEL_PATH):
        print("Downloading model from GitHub release...")
        response = requests.get(MODEL_URL, stream=True)
        if response.status_code == 200:
            with open(MODEL_PATH, 'wb') as f:
                for chunk in response.iter_content(chunk_size=8192):
                    f.write(chunk)
            print("Model downloaded successfully.")
        else:
            raise Exception("Failed to download model.")

download_model()


# loading the model after downloading from the releases 
model = tf.keras.models.load_model(MODEL_PATH)

model = tf.keras.models.load_model('model/my_model.keras')

labels = {0: 'Healthy', 1: 'Powdery', 2: 'Rust'}


remedy_videos = {
    'Healthy': None,
    'Rust': "https://www.youtube.com/embed/AKY_pelBZek",
    'Powdery': "https://www.youtube.com/embed/4GYpcXncLCg"
}

def allowed_file(filename):
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

def predict_stress(image_path):
    img = load_img(image_path, target_size=(225, 225))
    x = img_to_array(img) / 255.0
    x = np.expand_dims(x, axis=0)
    preds = model.predict(x)
    class_index = np.argmax(preds)
    label = labels.get(class_index, "Unknown")
    confidence = preds[0][class_index]
    
    suggestions = {
        'Healthy': "Your plant looks healthy! Keep up the good care.",
        'Rust': "Rust detected! Remove infected leaves and apply fungicide.",
        'Powdery': "Powdery mildew detected! Increase air circulation and use neem oil spray."
    }
    
    return label, confidence, suggestions.get(label, "No suggestion available.")

def send_email_alert(label, confidence, image_path):
    # Customize these with your email details
    EMAIL_ADDRESS = 'senderemail@gmail.com' #change the email as the sender email of your choice 
    EMAIL_PASSWORD = 'tvpm uaif eeyx tyof'  # Use app password for Gmail
    
    msg = EmailMessage()
    msg['Subject'] = f'Plant Stress Alert: {label}'
    msg['From'] = EMAIL_ADDRESS
    msg['To'] = 'receiver@gmail.com'  # Change to the email you want to send the mail this is temporary we will update it with login system for more comfortable use 
    
    body = f"""
    Stress detection alert:
    - Predicted Stress Type: {label}
    - Confidence: {confidence*100:.2f}%
    - Image: {image_path}
    
    Please take necessary action.
    """
    msg.set_content(body)
    
    try:
        with smtplib.SMTP_SSL('smtp.gmail.com', 465) as smtp:
            smtp.login(EMAIL_ADDRESS, EMAIL_PASSWORD)
            smtp.send_message(msg)
        print("Email sent successfully!")
    except Exception as e:
        print(f"Failed to send email: {e}")

def save_history(filename, label, confidence):
    entry = {
        'filename': filename,
        'label': label,
        'confidence': f"{confidence*100:.2f}%",
        'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    }
    if os.path.exists(HISTORY_FILE):
        with open(HISTORY_FILE, 'r') as f:
            history = json.load(f)
    else:
        history = []
    
    history.append(entry)
    with open(HISTORY_FILE, 'w') as f:
        json.dump(history, f, indent=4)

@app.route('/', methods=['GET', 'POST'])
def index():
    history = []
    if os.path.exists(HISTORY_FILE):
        with open(HISTORY_FILE, 'r') as f:
            history = json.load(f)
    
    if request.method == 'POST':
        if 'file' not in request.files:
            return redirect(request.url)
        
        file = request.files['file']
        if file.filename == '':
            return redirect(request.url)
        
        if file and allowed_file(file.filename):
            filename = secure_filename(file.filename)
            save_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
            file.save(save_path)

            label, confidence, suggestion = predict_stress(save_path)

            # Saves uploaded history
            save_history(filename, label, confidence)

            # Sends email alert 
            send_email_alert(label, confidence, save_path)

            # Passes video URL
            video_url = remedy_videos.get(label)

            return render_template('index.html', filename=filename, label=label,
                                   confidence=confidence*100, suggestion=suggestion,
                                   video_url=video_url, history=history)

    return render_template('index.html', history=history)

if __name__ == '__main__':
    os.makedirs(UPLOAD_FOLDER, exist_ok=True)
    app.run(host='0.0.0.0', port=5000, debug=True)

