# 🌿 Plant Stress Detection Using CNN

## Project Description

This repository presents a complete deep learning-powered system for the automated classification of plant leaf conditions, addressing the critical challenge of early disease detection in agriculture. At the core of the system is a custom-built Convolutional Neural Network (CNN), trained to identify three classes of leaf health: Healthy, Rust-infected, and Powdery Mildew-infected. The model is encapsulated within a responsive Flask web application, offering users a seamless interface to upload images, receive real-time predictions, and access targeted treatment suggestions. 

The pipeline ensures consistency through rigorous preprocessing and data augmentation techniques. It is optimized not only for accuracy but also for performance, making it suitable for deployment in edge environments or low-bandwidth scenarios. The interface is lightweight, browser-accessible, and designed to be intuitive for end-users, regardless of technical expertise.

## Project Vision

This project was developed with the vision of bridging the divide between advanced machine learning techniques and practical agricultural deployment. The primary aim is to create a system that can augment traditional plant disease management practices by providing intelligent, automated, and data-driven insights directly to farmers, agronomists, and researchers. In a world where food security increasingly relies on precision agriculture, the integration of AI into field-ready tools holds transformative potential. 

By embedding a performant deep learning model into a web-accessible, user-centered platform, this work offers a prototype that aligns with the broader goals of sustainable agriculture, reduced crop loss, and smarter, faster decision-making in the field.

## Future Scope

Looking ahead, the system is poised for a range of enhancements that could expand both its reach and impact. One of the most promising directions is to migrate the model to mobile-first platforms, enabling offline inference using TensorFlow Lite or similar frameworks, making it suitable for remote areas with limited connectivity. The incorporation of voice-based interfaces and multilingual support could further enhance accessibility for a wider audience, particularly in rural and non-English-speaking regions.

A robust cloud integration could facilitate continuous learning through real-time data feedback, while an expanded dataset encompassing more crop species and diverse infection patterns would improve generalizability. Coupling the application with IoT-enabled sensors or drone-based imaging systems would elevate it to a fully autonomous crop monitoring solution. These directions all converge on a singular goal: delivering an intelligent, scalable, and field-deployable system for plant stress detection and management.

## Key Features

The system encapsulates a full-stack machine learning deployment cycle—from dataset preprocessing and model training to API integration and UI/UX design. At its core, the CNN architecture has been designed for high-resolution image classification with minimal computational overhead. The web interface supports image input through drag-and-drop or file selection, after which the backend handles prediction, result rendering, and history logging. Users receive immediate feedback in the form of predicted class labels, corresponding care recommendations, and optional access to supplementary treatment videos.

The application also includes support for email notifications, allowing users to receive prediction results directly in their inbox for later reference or expert review. The backend is modular and maintainable, facilitating easy upgrades to newer model architectures such as EfficientNet or MobileNet as needed. The front-end experience remains simple yet effective, ensuring that users from non-technical backgrounds can engage with the system effortlessly.

- 🌱 Classifies leaf images into Healthy, Rust, or Powdery Mildew
- 🧠 Built using a compact yet powerful CNN model
- 🖼️ Real-time image upload and classification
- 💬 Provides actionable treatment suggestions
- 💌 Optional email alerts with prediction details
- 📊 Maintains prediction history for user reference
- 🔧 Easily deployable on low-resource devices and browsers


## Web Application Screenshot

![image](https://github.com/user-attachments/assets/743a4893-32f4-453e-9a8f-4444f8aad79e)
![image](https://github.com/user-attachments/assets/c1e63eb8-00a6-4a26-ae93-c33d9c65e4e4)
![image](https://github.com/user-attachments/assets/7680dd28-472a-4d62-9db5-09a591529577)
![image](https://github.com/user-attachments/assets/0ebc0eed-1d43-4284-bb37-512e1e822905)




