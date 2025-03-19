import argparse
import torch
import torch.nn as nn
import numpy as np
import os
from torchvision import transforms, datasets, models
from torch.utils.data import DataLoader
from PIL import Image
import sys

# ==== CONFIGURATION ==== 
IMG_SIZE = 224
BATCH_SIZE = 16

# ==== PARSE ARGUMENTS ==== 
parser = argparse.ArgumentParser(description="Ilmenite Machine Learning Model")
parser.add_argument("--model-path", type=str, default="/home/nova/nova/src/other/ilmenite_ml/models/best_model.pth", help="Path to the model file (.pth)")
parser.add_argument("--data-path", type=str, default="/home/nova/nova/src/other/ilmenite_ml/images", help="Path to the directory containing input images")
#parser.add_argument("--device", type=str, choices=["cpu", "cuda"], default="cuda" if torch.cuda.is_available() else "cpu", help="Device to run predictions on (cuda or cpu)")
parser.add_argument("--resnet-model", type=int, choices=[18, 34], default=18, help="ResNet model (18 or 34)")
args = parser.parse_args()

# ==== ARGUMENT VARIABLES ====
MODEL_PATH = args.model_path
DATA_DIR = args.data_dir
DEVICE = torch.device("cpu")
RESNET_MODEL = args.resnet_model

print(f"MODEL_PATH: {MODEL_PATH}")
print(f"DATA_PATH: {DATA_DIR}")
print(f"DEVICE: {DEVICE}")
print(f"RESNET_MODEL: ResNet-{RESNET_MODEL}")

# ==== LOAD MODEL ==== 
class ResNetRegression(nn.Module):
    def __init__(self):
        if RESNET_MODEL == 18:
            super(ResNetRegression, self).__init__()
            self.model = models.resnet18(weights=models.ResNet18_Weights.DEFAULT)
        elif RESNET_MODEL == 34:
            super(ResNetRegression, self).__init__()
            self.model = models.resnet34(weights=models.ResNet34_Weights.DEFAULT)
        self.model.fc = nn.Linear(512, 1)  # Modify final layer for regression (1 output)

    def forward(self, x):
        return self.model(x)

model = ResNetRegression().to(DEVICE)
model.load_state_dict(torch.load(MODEL_PATH, map_location=DEVICE))
model.eval()

# ==== DATA LOADING & TRANSFORM ==== 
transform = transforms.Compose([
    transforms.Resize((IMG_SIZE, IMG_SIZE)), 
    transforms.ToTensor(),
    transforms.Normalize(mean=[0.5, 0.5, 0.5], std=[0.5, 0.5, 0.5])
])

# Load the images from the directory
image_paths = [
    os.path.join(DATA_DIR, f)
    for f in os.listdir(DATA_DIR)
    if os.path.isfile(os.path.join(DATA_DIR, f)) and not f.endswith((".md", ".gitkeep"))
]

# Function to predict on all images
def predict_all_images(model, image_paths, transform):
    predictions = []
    
    with torch.no_grad():
        for img_path in image_paths:
            img = transform(Image.open(img_path).convert("RGB")).unsqueeze(0).to(DEVICE)
            output = model(img)
            pred_label = output.item()
            predictions.append(pred_label)
    
    return predictions

# Predict on all images in the directory
predictions = predict_all_images(model, image_paths, transform)

# ==== STATISTICS ==== 
def calculate_statistics(predictions):
    avg_pred = np.mean(predictions)
    stdev_pred = np.std(predictions)
    return avg_pred, stdev_pred

# Calculate and display statistics
avg_pred, stdev_pred = calculate_statistics(predictions)

print("Predictions for all images:")
for idx, pred in enumerate(predictions):
    print(f"Image {idx+1}: {pred:.2f}%")

print("")
print(f"Average: {avg_pred:.2f}%")
print(f"Standard Deviation: {stdev_pred:.2f}%")
