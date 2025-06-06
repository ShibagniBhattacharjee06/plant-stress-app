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
import requests

def download_file(url, dest):
    if not os.path.exists(dest):
        print(f"Downloading model from {url}...")
        response = requests.get(url)
        with open(dest, 'wb') as f:
            f.write(response.content)
        print("Download complete.")
    else:
        print(f"Model file {dest} already exists.")

MODEL_URL = 'https://drive.google.com/uc?export=download&id=1sGxxq3F9T-GdboMQ5knptNlHk2_6-rbb'
MODEL_PATH = 'model/my_model.keras'

os.makedirs('model', exist_ok=True)
download_file(MODEL_URL, MODEL_PATH)

# Now load the model after download is ensured
model = tf.keras.models.load_model(MODEL_PATH)

# Load your model
model = tf.keras.models.load_model('model/my_model.keras')

labels = {0: 'Healthy', 1: 'Powdery', 2: 'Rust'}

# Remedy videos URLs (YouTube embeds or links)
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
    EMAIL_ADDRESS = 'bhattacharjeesagshi1234@gmail.com'
    EMAIL_PASSWORD = 'tvpm uaif eeyx tyof'  # Use app password for Gmail
    
    msg = EmailMessage()
    msg['Subject'] = f'Plant Stress Alert: {label}'
    msg['From'] = EMAIL_ADDRESS
    msg['To'] = 'bhattacharjeeshibagni85@gmail.com'  # Change to your target email
    
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

            # Save upload history
            save_history(filename, label, confidence)

            # Send email alert (optional, comment if you want)
            send_email_alert(label, confidence, save_path)

            # Pass video URL
            video_url = remedy_videos.get(label)

            return render_template('index.html', filename=filename, label=label,
                                   confidence=confidence*100, suggestion=suggestion,
                                   video_url=video_url, history=history)

    return render_template('index.html', history=history)

if __name__ == '__main__':
    os.makedirs(UPLOAD_FOLDER, exist_ok=True)
    app.run(host='0.0.0.0', port=5000, debug=True)

