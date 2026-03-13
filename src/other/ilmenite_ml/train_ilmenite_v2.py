import os
import torch
import torch.nn as nn
import torch.optim as optim
import torchvision.transforms as transforms
import torchvision.models as models
import torchvision.datasets as datasets
from torch.utils.data import DataLoader, random_split
from tqdm import tqdm
from datetime import datetime

# ==== CONFIGURATION ====
DATA_DIR = "datasets"
SAVE_DIR = "models"
BATCH_SIZE = 32
LR = 0.0001
EPOCHS = 100 # or 50
IMG_SIZE = 224
RESNET_MODEL = 18 # or 34
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")

# create class to add gaussian noise
class AddGaussianNoise(object):
    def __init__(self, mean=0.0, std=0.02):
        self.mean = mean
        self.std = std

    def __call__(self, tensor):
        noise = torch.randn_like(tensor) * self.std + self.mean
        return torch.clamp(tensor + noise, 0.0, 1.0)

    def __repr__(self):
        return f"{self.__class__.__name__}(mean={self.mean}, std={self.std})"


# ==== CREATE TIMESTAMPED DIRECTORY ====
timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
run_dir = os.path.join(SAVE_DIR, f"run_{timestamp}")
os.makedirs(run_dir, exist_ok=True)

# ==== DATA AUGMENTATION & NORMALISATION ====
# TODO: add gausian noise and cropping transforms too
train_transform = transforms.Compose([
    transforms.RandomResizedCrop(
        IMG_SIZE,
        scale=(0.8, 1.0),   # crop between 80–100% of image area
        ratio=(0.9, 1.1)    # keep aspect ratio roughly intact
    ),
    transforms.RandomHorizontalFlip(),
    transforms.RandomRotation(20),
    transforms.ColorJitter(brightness=0.2, contrast=0.2),
    transforms.ToTensor(),    
    AddGaussianNoise(mean=0.0, std=0.02),
    transforms.Normalize(
        mean=[0.485, 0.456, 0.406],
        std=[0.229, 0.224, 0.225]
    )
])

# don't want to augment the validation data
val_transform = transforms.Compose([
    transforms.Resize(IMG_SIZE),
    transforms.CenterCrop(IMG_SIZE),
    transforms.ToTensor(),
    transforms.Normalize([0.485, 0.456, 0.406],
                         [0.229, 0.224, 0.225])
])


# ==== DATA LOADING ====

# Load dataset without transforms first
full_dataset = datasets.ImageFolder(root=DATA_DIR)

# Convert folder names to float labels (e.g., "5%" → 5.0)
idx_to_class = {v: k for k, v in full_dataset.class_to_idx.items()}
full_dataset.samples = [
    (img, float(idx_to_class[label].replace('%', '')) / 100.0) # normalise data to be 0-1
    
    for img, label in full_dataset.samples
]

# Split into training (80%) and validation (20%)
train_size = int(0.8 * len(full_dataset))
val_size = len(full_dataset) - train_size
train_dataset, val_dataset = random_split(full_dataset, [train_size, val_size])

# Assign transforms
train_dataset.dataset.transform = train_transform
val_dataset.dataset.transform = val_transform

# DataLoaders
train_loader = DataLoader(
    train_dataset,
    batch_size=BATCH_SIZE,
    shuffle=True,
    num_workers=0,
    pin_memory=True
)

val_loader = DataLoader(
    val_dataset,
    batch_size=BATCH_SIZE,
    shuffle=False,
    num_workers=0,
    pin_memory=True
)

# ==== RESNET-18 MODEL (MODIFIED FOR REGRESSION) ====
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

# ==== LOSS & OPTIMISER ====
criterion = nn.MSELoss()  # Mean Squared Error for regression
optimizer = optim.Adam(model.parameters(), lr=LR)

# ==== LEARNING RATE SCHEDULER ====
scheduler = optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='min', factor=0.1, patience=5, verbose=True)

# ==== EARLY STOPPING CONFIG ====
early_stop_patience = 10
early_stop_counter = 0

# ==== TRAINING FUNCTION ====
def train(model, train_loader, val_loader, epochs, criterion, optimizer, scheduler, device, save_path):
    best_val_loss = float('inf')
    best_model_path = os.path.join(save_path, "best_model.pth")
    print("")

    for epoch in range(epochs):
        model.train()
        total_loss = 0
        for images, labels in tqdm(train_loader, desc=f"Epoch {epoch+1}/{epochs} Training", unit="batch"):
            images, labels = images.to(device).float(), labels.to(device).float().unsqueeze(1)  
            optimizer.zero_grad()
            outputs = model(images)
            loss = criterion(outputs, labels)
            loss.backward()
            optimizer.step()
            total_loss += loss.item()

        # ==== VALIDATION ====
        model.eval()
        val_loss = 0
        with torch.no_grad():
            for images, labels in tqdm(val_loader, desc=f"Epoch {epoch+1}/{epochs} Validation", unit="batch"):
                images, labels = images.to(device).float(), labels.to(device).float().unsqueeze(1)
                outputs = model(images)
                loss = criterion(outputs, labels)
                val_loss += loss.item()

        avg_train_loss = total_loss / len(train_loader)
        avg_val_loss = val_loss / len(val_loader)

        # Print progress
        print(f"Epoch [{epoch+1}/{epochs}] - Train Loss: {avg_train_loss:.4f} - Val Loss: {avg_val_loss:.4f}")

        # ==== LEARNING RATE SCHEDULING ====
        scheduler.step(avg_val_loss)

        # ==== SAVE EVERY 5 EPOCHS ====
        if (epoch + 1) % 5 == 0:
            save_checkpoint_path = os.path.join(save_path, f"epoch_{epoch+1}.pth")
            torch.save(model.state_dict(), save_checkpoint_path)
            print(f"Checkpoint saved: {save_checkpoint_path}")

        # ==== SAVE BEST MODEL ====
        global early_stop_counter
        if avg_val_loss < best_val_loss:
            best_val_loss = avg_val_loss
            torch.save(model.state_dict(), best_model_path)
            print(f"Best model updated: {best_model_path} (Val Loss: {best_val_loss:.4f})")
            early_stop_counter = 0 
        else:
            early_stop_counter += 1

        # ==== EARLY STOPPING ====
        if early_stop_counter >= early_stop_patience:
            print(f"Early stopping triggered at epoch {epoch+1}. Training stopped.")
            break

# ==== START TRAINING ====
train(model, train_loader, val_loader, EPOCHS, criterion, optimizer, scheduler, DEVICE, run_dir)

# ==== FINAL MESSAGE ====
print(f"Training complete. Best model saved at: {os.path.join(run_dir, 'best_model.pth')}")
