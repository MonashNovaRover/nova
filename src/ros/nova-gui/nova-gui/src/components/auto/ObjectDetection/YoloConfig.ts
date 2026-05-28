export type YOLOOutputFormat =
  | "xyxy"  // [x1, y1, x2, y2, confidence, classId] - corner coordinates
  | "xywh"  // [x_center, y_center, width, height, confidence, classId] - center coordinates

export interface YOLOConfig {
  id: string
  label: string
  modelName: string
  classNames: string[]
  outputFormat: YOLOOutputFormat
}

type YoloConfigWithoutId = Omit<YOLOConfig, "id">;

export const YoloConfigs = {
  // Coco pretrained test YOLO model
  coco: {
    label: "COCO (yolo26n)",
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
  },
  urcDino: {
    label: "URC Dino",
    modelName: "urc-dino.onnx",
    classNames: [
      "bottle",
      "hammer_pick",
      "mallet",
    ],
    outputFormat: "xyxy",  // Corner coordinates format
  },
  urcEchidna: {
    label: "URC Echidna",
    modelName: "urc-echidna.onnx",
    classNames: [
      "bottle",
      "hammer_pick",
      "mallet",
    ],
    outputFormat: "xyxy",  // Corner coordinates format
  },
} satisfies Record<string, YoloConfigWithoutId>;

export type YoloModelId = keyof typeof YoloConfigs;

export const DEFAULT_YOLO_MODEL_ID: YoloModelId = "urcDino";

export function getYoloConfig(modelId: string | undefined): YOLOConfig {
  const resolvedId =
    modelId && modelId in YoloConfigs
      ? (modelId as YoloModelId)
      : DEFAULT_YOLO_MODEL_ID;

  return {
    id: resolvedId,
    ...YoloConfigs[resolvedId],
  };
}

export const YoloModelOptions = Object.entries(YoloConfigs).map(([id, config]) => ({
  id,
  label: config.label,
  modelName: config.modelName,
}));
