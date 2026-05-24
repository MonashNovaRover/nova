import {Button} from "@nextui-org/react";
import {Pause, Play, Square} from "react-feather";
import {SerialPreset} from "../../../views/shared/CamerasPage/CameraProfileConstants.ts";

interface SerialPresetControlsProps {
  presets: SerialPreset[];
  startStreaming: (serials: string[], useAllMessage: boolean) => void;
  pauseStreaming: (serials: string[], useAllMessage: boolean) => void;
  stopStreaming: (serials: string[], useAllMessage: boolean) => void;
}

export const SerialPresetControls = ({
  presets,
  startStreaming,
  pauseStreaming,
  stopStreaming,
}: SerialPresetControlsProps) => {
  return (
    <div className="flex flex-col gap-2">
      <span>Camera Set Controls</span>
      {presets.map((preset) => (
        <div key={preset.displayName} className="flex flex-row items-center justify-between">
          <span className="text-sm text-default-500">{preset.displayName}</span>
          <div className="flex flex-row gap-2">
            <Button
              color="primary"
              size="md"
              startContent={<Play size={16}/>}
              onPress={() => startStreaming(preset.serials, false)}
            >
              Start
            </Button>
            <Button
              color="warning"
              size="md"
              startContent={<Pause size={16}/>}
              onPress={() => pauseStreaming(preset.serials, false)}
            >
              Pause
            </Button>
            <Button
              color="danger"
              size="md"
              startContent={<Square size={16}/>}
              onPress={() => stopStreaming(preset.serials, false)}
            >
              Stop
            </Button>
          </div>
        </div>
      ))}
    </div>
  );
};
