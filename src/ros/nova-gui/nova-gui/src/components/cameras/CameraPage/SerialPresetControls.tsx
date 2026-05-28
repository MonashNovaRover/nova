import {Button, Switch, Tooltip, Tab, Tabs} from "@nextui-org/react";
import {Pause, Play, Square} from "react-feather";
import {SerialPreset, SerialPresetGroup} from "../../../views/shared/CamerasPage/CameraViewConstants.tsx";
import {useState} from "react";

interface SerialPresetControlsProps {
  presetGroups: SerialPresetGroup[];
  startStreaming: (serials: string[], useAllMessage: boolean) => void;
  pauseStreaming: (serials: string[], useAllMessage: boolean) => void;
  stopStreaming: (serials: string[], useAllMessage: boolean) => void;
}

export const SerialPresetControls = ({
  presetGroups,
  startStreaming,
  pauseStreaming,
  stopStreaming,
}: SerialPresetControlsProps) => {
  const [activeGroup, setActiveGroup] = useState(presetGroups[0].groupName);
  const [activeTask, setActiveTask] = useState("");

  const activeGroupData = presetGroups.find((g) => g.groupName === activeGroup) ?? presetGroups[0];

  const handleToggleTask = (preset: SerialPreset, isSelected: boolean) => {
    if (isSelected) {
      // Only stop cameras that were in the previous task but not in the new one
      if (activeTask) {
        const prevPreset = activeGroupData.presets.find((p) => p.displayName === activeTask);
        if (prevPreset) {
          const toStop = prevPreset.serials.filter((s) => !preset.serials.includes(s));
          if (toStop.length > 0) {
            stopStreaming(toStop, false);
          }
        }
      }
      // Start cameras in the new task that weren't already running
      const prevSerials = activeTask
        ? activeGroupData.presets.find((p) => p.displayName === activeTask)?.serials ?? []
        : [];
      const toStart = preset.serials.filter((s) => !prevSerials.includes(s));
      if (toStart.length > 0) {
        startStreaming(toStart, false);
      }
      setActiveTask(preset.displayName);
    } else {
      stopStreaming(preset.serials, false);
      setActiveTask("");
    }
  };

  return (
    <div className="flex flex-col gap-2">
      <span>Camera Set Controls</span>
      {presetGroups.length > 1 && (
        <Tabs
          size="sm"
          variant="bordered"
          selectedKey={activeGroup}
          onSelectionChange={(key) => {
            setActiveGroup(key as string);
            setActiveTask("");
          }}
        >
          {presetGroups.map((group) => (
            <Tab key={group.groupName} title={group.groupName} />
          ))}
        </Tabs>
      )}
      {activeGroupData.presets.map((preset, idx) => (
        <div key={preset.displayName} className="flex flex-col gap-2">
          {preset.section && (idx === 0 || activeGroupData.presets[idx - 1]?.section !== preset.section) && (
            <span className={`text-xs font-semibold text-default-400 uppercase tracking-wide ${idx > 0 ? "mt-2" : "mt-1"}`}>
              {preset.section}
            </span>
          )}
          <div className="flex flex-row items-center justify-between">
            <Tooltip
              className="dark text-foreground"
              content={preset.serials.join(", ")}
              closeDelay={100}
            >
              <span className={`text-sm ${activeGroupData.mode === "toggle" && activeTask === preset.displayName ? "text-primary font-semibold" : "text-default-500"}`}>
                {preset.displayName}
              </span>
            </Tooltip>
            {activeGroupData.mode === "toggle" ? (
              <Switch
                size="sm"
                color="primary"
                isSelected={activeTask === preset.displayName}
                onValueChange={(isSelected) => handleToggleTask(preset, isSelected)}
              />
            ) : (
              <div className="flex flex-row gap-2">
                <Button
                  color="primary"
                  size="sm"
                  startContent={<Play size={16}/>}
                  onPress={() => startStreaming(preset.serials, false)}
                >
                  Start
                </Button>
                <Button
                  color="warning"
                  size="sm"
                  startContent={<Pause size={16}/>}
                  onPress={() => pauseStreaming(preset.serials, false)}
                >
                  Pause
                </Button>
                <Button
                  color="danger"
                  size="sm"
                  startContent={<Square size={16}/>}
                  onPress={() => stopStreaming(preset.serials, false)}
                >
                  Stop
                </Button>
              </div>
            )}
          </div>
        </div>
      ))}
    </div>
  );
};
