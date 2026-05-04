import { Select, SelectItem, SharedSelection, Switch } from "@nextui-org/react";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";
import { getYoloConfig, YoloModelOptions } from "../../auto/ObjectDetection/YoloConfig.ts";

/**
 * YOLO runtime settings component.
 * @constructor
 */
export function YoloSettings() {
  const [yoloActiveModel, setYoloActiveModel] = useGenericStore<string>("yoloActiveModel");
  const [yoloUseWebGPU, setYoloUseWebGPU] = useGenericStore<boolean>("yoloUseWebGPU");
  const [yoloTimingLogs, setYoloTimingLogs] = useGenericStore<boolean>("yoloTimingLogs");
  const activeModel = getYoloConfig(yoloActiveModel);

  const onModelSelectionChange = (keys: SharedSelection) => {
    const selected = Array.from(keys)[0];
    if (typeof selected === "string") {
      setYoloActiveModel(selected);
    }
  };

  return (
    <div className="flex flex-col gap-3 mt-2 mb-4">
      <Select
        size="sm"
        label="Active YOLO Model"
        selectedKeys={[activeModel.id]}
        onSelectionChange={onModelSelectionChange}
      >
        {YoloModelOptions.map((option) => (
          <SelectItem key={option.id}>
            {option.label}
          </SelectItem>
        ))}
      </Select>

      <div className="text-xs text-default-500">
        Current model file: {activeModel.modelName}
      </div>

      <Switch
        size="sm"
        isSelected={yoloUseWebGPU}
        onValueChange={setYoloUseWebGPU}
      >
        Prefer WebGPU for YOLO
      </Switch>

      <Switch
        size="sm"
        isSelected={yoloTimingLogs}
        onValueChange={setYoloTimingLogs}
      >
        Enable YOLO timing logs
      </Switch>

      <div className="text-xs text-default-500">
        Logs include batch, preprocessms, runms, postms, and totalms in the browser console.
      </div>
    </div>
  );
}
