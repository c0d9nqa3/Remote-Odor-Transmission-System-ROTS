#!/usr/bin/env python3
"""
ROTS AI Model Training Script
Remote Odor Transmission System - Machine Learning Model

This script trains a neural network model for odor classification using
sensor data from MQ-series gas sensors and environmental sensors.

Author: ROTS Team
Date: 2024
"""

import numpy as np
import pandas as pd
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
import matplotlib.pyplot as plt
import seaborn as sns
import joblib
import json
import os

# Set random seeds for reproducibility
np.random.seed(42)
tf.random.set_seed(42)

class ROTSModelTrainer:
    def __init__(self):
        self.model = None
        self.scaler = StandardScaler()
        self.feature_names = [
            'mq2_value', 'mq3_value', 'mq4_value', 'mq5_value',
            'mq6_value', 'mq7_value', 'mq8_value', 'mq9_value',
            'temperature', 'humidity', 'pressure',
            'mq2_mq3_ratio', 'mq4_mq5_ratio', 'mq6_mq7_ratio', 'mq8_mq9_ratio'
        ]
        self.odor_types = ['coffee', 'alcohol', 'lemon', 'mint', 'lavender', 'mixed']
        self.num_classes = len(self.odor_types)
        
    def generate_synthetic_data(self, num_samples=5000):
        """
        Generate synthetic training data for demonstration purposes.
        In a real implementation, this would load actual sensor data.
        """
        print("Generating synthetic training data...")
        
        data = []
        labels = []
        
        for i in range(num_samples):
            # Generate base sensor readings
            base_readings = np.random.normal(0.5, 0.2, 8)  # MQ sensors
            
            # Add environmental factors
            temperature = np.random.normal(25.0, 5.0)
            humidity = np.random.normal(50.0, 15.0)
            pressure = np.random.normal(1013.25, 10.0)
            
            # Select odor type
            odor_type = np.random.choice(self.num_classes)
            
            # Modify sensor readings based on odor type
            if odor_type == 0:  # Coffee
                base_readings[0] += np.random.normal(0.3, 0.1)  # MQ2 (combustible gas)
                base_readings[1] += np.random.normal(0.1, 0.05)  # MQ3 (alcohol)
            elif odor_type == 1:  # Alcohol
                base_readings[1] += np.random.normal(0.4, 0.1)  # MQ3 (alcohol)
                base_readings[2] += np.random.normal(0.2, 0.05)  # MQ4 (methane)
            elif odor_type == 2:  # Lemon
                base_readings[2] += np.random.normal(0.3, 0.1)  # MQ4 (methane)
                base_readings[3] += np.random.normal(0.2, 0.05)  # MQ5 (LPG)
            elif odor_type == 3:  # Mint
                base_readings[3] += np.random.normal(0.3, 0.1)  # MQ5 (LPG)
                base_readings[4] += np.random.normal(0.2, 0.05)  # MQ6 (LPG)
            elif odor_type == 4:  # Lavender
                base_readings[4] += np.random.normal(0.3, 0.1)  # MQ6 (LPG)
                base_readings[5] += np.random.normal(0.2, 0.05)  # MQ7 (CO)
            elif odor_type == 5:  # Mixed
                # Mixed odor has multiple elevated sensors
                base_readings += np.random.normal(0.2, 0.1, 8)
            
            # Add some noise
            base_readings += np.random.normal(0, 0.05, 8)
            
            # Calculate cross-ratios
            ratios = [
                base_readings[0] / (base_readings[1] + 1e-6),  # MQ2/MQ3
                base_readings[2] / (base_readings[3] + 1e-6),  # MQ4/MQ5
                base_readings[4] / (base_readings[5] + 1e-6),  # MQ6/MQ7
                base_readings[6] / (base_readings[7] + 1e-6)   # MQ8/MQ9
            ]
            
            # Combine all features
            features = np.concatenate([base_readings, [temperature, humidity, pressure], ratios])
            
            data.append(features)
            labels.append(odor_type)
        
        return np.array(data), np.array(labels)
    
    def create_model(self):
        """Create the neural network model architecture"""
        print("Creating model architecture...")
        
        model = keras.Sequential([
            layers.Input(shape=(15,), name='sensor_input'),
            
            # Feature extraction layers
            layers.Dense(64, activation='relu', name='dense_1'),
            layers.BatchNormalization(name='batch_norm_1'),
            layers.Dropout(0.3, name='dropout_1'),
            
            layers.Dense(32, activation='relu', name='dense_2'),
            layers.BatchNormalization(name='batch_norm_2'),
            layers.Dropout(0.3, name='dropout_2'),
            
            layers.Dense(16, activation='relu', name='dense_3'),
            layers.Dropout(0.2, name='dropout_3'),
            
            # Classification layer
            layers.Dense(self.num_classes, activation='softmax', name='output')
        ])
        
        # Compile model
        model.compile(
            optimizer=keras.optimizers.Adam(learning_rate=0.001),
            loss='sparse_categorical_crossentropy',
            metrics=['accuracy']
        )
        
        return model
    
    def train_model(self, X_train, y_train, X_val, y_val, epochs=100, batch_size=32):
        """Train the model"""
        print("Training model...")
        
        self.model = self.create_model()
        
        # Callbacks
        callbacks = [
            keras.callbacks.EarlyStopping(
                monitor='val_accuracy',
                patience=10,
                restore_best_weights=True
            ),
            keras.callbacks.ReduceLROnPlateau(
                monitor='val_loss',
                factor=0.5,
                patience=5,
                min_lr=1e-6
            )
        ]
        
        # Train model
        history = self.model.fit(
            X_train, y_train,
            validation_data=(X_val, y_val),
            epochs=epochs,
            batch_size=batch_size,
            callbacks=callbacks,
            verbose=1
        )
        
        return history
    
    def evaluate_model(self, X_test, y_test):
        """Evaluate model performance"""
        print("Evaluating model...")
        
        # Predictions
        y_pred = self.model.predict(X_test)
        y_pred_classes = np.argmax(y_pred, axis=1)
        
        # Calculate metrics
        accuracy = np.mean(y_pred_classes == y_test)
        
        print(f"Test Accuracy: {accuracy:.4f}")
        
        # Confusion matrix
        from sklearn.metrics import confusion_matrix, classification_report
        
        cm = confusion_matrix(y_test, y_pred_classes)
        print("\nConfusion Matrix:")
        print(cm)
        
        print("\nClassification Report:")
        print(classification_report(y_test, y_pred_classes, target_names=self.odor_types))
        
        return accuracy, cm
    
    def plot_training_history(self, history):
        """Plot training history"""
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 4))
        
        # Plot accuracy
        ax1.plot(history.history['accuracy'], label='Training Accuracy')
        ax1.plot(history.history['val_accuracy'], label='Validation Accuracy')
        ax1.set_title('Model Accuracy')
        ax1.set_xlabel('Epoch')
        ax1.set_ylabel('Accuracy')
        ax1.legend()
        
        # Plot loss
        ax2.plot(history.history['loss'], label='Training Loss')
        ax2.plot(history.history['val_loss'], label='Validation Loss')
        ax2.set_title('Model Loss')
        ax2.set_xlabel('Epoch')
        ax2.set_ylabel('Loss')
        ax2.legend()
        
        plt.tight_layout()
        plt.savefig('training_history.png', dpi=300, bbox_inches='tight')
        plt.show()
    
    def plot_confusion_matrix(self, cm):
        """Plot confusion matrix"""
        plt.figure(figsize=(8, 6))
        sns.heatmap(cm, annot=True, fmt='d', cmap='Blues',
                   xticklabels=self.odor_types,
                   yticklabels=self.odor_types)
        plt.title('Confusion Matrix')
        plt.xlabel('Predicted')
        plt.ylabel('Actual')
        plt.tight_layout()
        plt.savefig('confusion_matrix.png', dpi=300, bbox_inches='tight')
        plt.show()
    
    def save_model(self, model_path='rots_model'):
        """Save the trained model and preprocessing objects"""
        print("Saving model...")
        
        # Create models directory if it doesn't exist
        os.makedirs('models', exist_ok=True)
        
        # Save TensorFlow model
        self.model.save(f'{model_path}.h5')
        
        # Save scaler
        joblib.dump(self.scaler, f'{model_path}_scaler.pkl')
        
        # Save model metadata
        metadata = {
            'feature_names': self.feature_names,
            'odor_types': self.odor_types,
            'num_classes': self.num_classes,
            'input_shape': [15],
            'model_type': 'neural_network'
        }
        
        with open(f'{model_path}_metadata.json', 'w') as f:
            json.dump(metadata, f, indent=2)
        
        print(f"Model saved to {model_path}.h5")
        print(f"Scaler saved to {model_path}_scaler.pkl")
        print(f"Metadata saved to {model_path}_metadata.json")
    
    def convert_to_tflite(self, model_path='rots_model'):
        """Convert model to TensorFlow Lite for ESP32 deployment"""
        print("Converting to TensorFlow Lite...")
        
        # Load the saved model
        model = keras.models.load_model(f'{model_path}.h5')
        
        # Convert to TensorFlow Lite
        converter = tf.lite.TFLiteConverter.from_keras_model(model)
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        converter.target_spec.supported_types = [tf.float16]
        
        # Convert
        tflite_model = converter.convert()
        
        # Save
        with open(f'{model_path}.tflite', 'wb') as f:
            f.write(tflite_model)
        
        print(f"TensorFlow Lite model saved to {model_path}.tflite")
        
        # Print model size
        model_size = len(tflite_model) / 1024
        print(f"Model size: {model_size:.2f} KB")
        
        return tflite_model

def main():
    """Main training function"""
    print("ROTS AI Model Training")
    print("=" * 50)
    
    # Initialize trainer
    trainer = ROTSModelTrainer()
    
    # Generate synthetic data
    X, y = trainer.generate_synthetic_data(num_samples=5000)
    
    # Split data
    X_train, X_temp, y_train, y_temp = train_test_split(
        X, y, test_size=0.3, random_state=42, stratify=y
    )
    X_val, X_test, y_val, y_test = train_test_split(
        X_temp, y_temp, test_size=0.5, random_state=42, stratify=y_temp
    )
    
    # Scale features
    X_train_scaled = trainer.scaler.fit_transform(X_train)
    X_val_scaled = trainer.scaler.transform(X_val)
    X_test_scaled = trainer.scaler.transform(X_test)
    
    print(f"Training set size: {X_train_scaled.shape}")
    print(f"Validation set size: {X_val_scaled.shape}")
    print(f"Test set size: {X_test_scaled.shape}")
    
    # Train model
    history = trainer.train_model(
        X_train_scaled, y_train,
        X_val_scaled, y_val,
        epochs=100,
        batch_size=32
    )
    
    # Evaluate model
    accuracy, cm = trainer.evaluate_model(X_test_scaled, y_test)
    
    # Plot results
    trainer.plot_training_history(history)
    trainer.plot_confusion_matrix(cm)
    
    # Save model
    trainer.save_model()
    
    # Convert to TensorFlow Lite
    trainer.convert_to_tflite()
    
    print("\nTraining completed successfully!")
    print(f"Final test accuracy: {accuracy:.4f}")

if __name__ == "__main__":
    main()
