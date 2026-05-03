import { Switch } from "@nextui-org/react";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";

/**
 * YOLO runtime settings component.
 * @constructor
 */
export function YoloSettings() {
  const [yoloUseWebGPU, setYoloUseWebGPU] = useGenericStore<boolean>("yoloUseWebGPU");
  const [yoloTimingLogs, setYoloTimingLogs] = useGenericStore<boolean>("yoloTimingLogs");

  return (
    <div className="flex flex-col gap-3 mt-2 mb-4">
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
