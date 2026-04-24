export type YOLOOutputFormat =
  | "xyxy"  // [x1, y1, x2, y2, confidence, classId] - corner coordinates
  | "xywh"  // [x_center, y_center, width, height, confidence, classId] - center coordinates

export interface YOLOConfig {
  modelName: string
  classNames: string[]
  outputFormat: YOLOOutputFormat
}

export const URCYOLOConfig : YOLOConfig = {
  modelName: "best.onnx",
  classNames: [
    "mallet",
    "hammer_pick",
    "bottle",
  ],
  outputFormat: "xyxy",  // Corner coordinates format
}

// Coco pretrained test YOLO model
export const CocoConfig : YOLOConfig = {
  modelName: "yolo26n.onnx",
  classNames: [
    "person",
    "bicycle",
    "car",
    "motorcycle",
    "airplane",
    "bus",
    "train",
    "truck",
    "boat",
    "traffic light",
    "fire hydrant",
    "stop sign",
    "parking meter",
    "bench",
    "bird",
    "cat",
    "dog",
    "horse",
    "sheep",
    "cow",
    "elephant",
    "bear",
    "zebra",
    "giraffe",
    "backpack",
    "umbrella",
    "handbag",
    "tie",
    "suitcase",
    "frisbee",
    "skis",
    "snowboard",
    "sports ball",
    "kite",
    "baseball bat",
    "baseball glove",
    "skateboard",
    "surfboard",
    "tennis racket",
    "bottle",
    "wine glass",
    "cup",
    "fork",
    "knife",
    "spoon",
    "bowl",
    "banana",
    "apple",
    "sandwich",
    "orange",
    "broccoli",
    "carrot",
    "hot dog",
    "pizza",
    "donut",
    "cake",
    "chair",
    "couch",
    "potted plant",
    "bed",
    "dining table",
    "toilet",
    "tv",
    "laptop",
    "mouse",
    "remote",
    "keyboard",
    "cell phone",
    "microwave",
    "oven",
    "toaster",
    "sink",
    "refrigerator",
    "book",
    "clock",
    "vase",
    "scissors",
    "teddy bear",
    "hair drier",
    "toothbrush",
  ],
  outputFormat: "xyxy",  // Original format with corner coordinates
}

// Active YOLO config: switch this to change the model/labels, then reload the page.
export const ActiveYoloConfig: YOLOConfig = URCYOLOConfig;
