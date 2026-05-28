import {Button, Card, CardBody, CardFooter, CardHeader, Input, Tooltip} from "@nextui-org/react";
import {Minus, Pause, Play, Plus, Square, X} from "react-feather";
import {BooleanChip} from "../CameraComponent/components/BooleanChip.tsx";
import {CamerasTable} from "./CamerasTable.tsx";
import {useRosNodes} from "../../../utils/hooks/useRosNodes.ts";
import {useEffect, useMemo} from "react";
import { useStreamingBifrost } from "../hooks/cameraBifrostHooks.ts";
import { useSelector } from "react-redux";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { RootState } from "../../../redux/RootState.ts";
import {ProfileOption} from "../../../views/shared/CamerasPage/CameraProfileConstants.ts";
import {CameraProfileSelector} from "./CameraProfileSelector.tsx";
import {SerialPresetControls} from "./SerialPresetControls.tsx";
import { SerialPresetGroup } from "../../../views/shared/CamerasPage/CameraViewConstants.tsx";

interface CameraSidebarProps {
  refreshAvailabilities: () => void;
  showSidebar: boolean;
  setShowSidebar: (_: boolean) => void
  gridSize: number
  setGridSize: (_: number) => void
  presets: ProfileOption[]
  serialPresetGroups?: SerialPresetGroup[]
}

export const CameraSidebar = (
  {refreshAvailabilities, showSidebar, setShowSidebar, gridSize, setGridSize, presets, serialPresetGroups}
  : CameraSidebarProps) => {
  const expandedSidebarWidth = "27vw";
  const sidebarWidth = showSidebar ? expandedSidebarWidth : "0px";

  const nodes = useRosNodes();
  const camerasRunning = useMemo(() => nodes.includes("/camera_streamer"), [nodes]);

  const booleanChip = useMemo(() => <BooleanChip
    boolean={camerasRunning}
    variant="dot"
    trueText="Cameras Running"
    falseText="Cameras Stopped"
    size="lg"
  />, [camerasRunning])

  const [startStreaming, pauseStreaming, stopStreaming] = useStreamingBifrost(refreshAvailabilities);
  const bifrost = useBifrost({ topic: RosTopic.CAMERAS });
  const onlineCameras = useSelector((state: RootState) => state.camerasStore.cameras);
  const onlineCameraSerials = onlineCameras.map((cam) => cam.serial);
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);
  
  return (
    <div
      className="transition-all duration-300 overflow-clip h-full"
      style={{
        maxWidth: sidebarWidth,
        width:sidebarWidth
      }}
    >
      <div
        className="top-0 bottom-0 left-0 right-0 h-full"
        style={{width: expandedSidebarWidth}}
      >
        <Card radius="none" className="h-full" style={{
          width: expandedSidebarWidth
        }}>
          <CardHeader>
                <span className="grow font-bold">
                   Control Panel
                </span>
            <Button isIconOnly size="sm" variant="light" onPressStart={() => setShowSidebar(false)}>
              <X size={20}/>
            </Button>
          </CardHeader>
          <CardBody className="flex flex-col gap-3 overflow-y-auto">
            <Tooltip
              className="dark text-foreground"
              content="Not Real Time"
              closeDelay={100}
            >
              {booleanChip}
            </Tooltip>

            <span>Streaming Controls</span>
            <div className="grid grid-cols-3 gap-2">
              <Button color="primary" size="sm" startContent={<Play size={14}/>} onPress={()=>startStreaming(onlineCameraSerials, true)}>
                Start all
              </Button>
              <Button color="warning" size="sm" startContent={<Pause size={14}/>} onPress={()=>pauseStreaming(onlineCameraSerials, true)}>
                Pause all
              </Button>
              <Button color="danger" size="sm" startContent={<Square size={14}/>} onPress={()=>stopStreaming(onlineCameraSerials, true)}>
                Stop all
              </Button>
            </div>

            <span>Set Preset</span>
            <CameraProfileSelector serials={onlineCameraSerials} options={presets}/>

            <span>Camera Status</span>
            <CamerasTable refreshAvailabilies={refreshAvailabilities}/>

            <div className="grow"></div>

          </CardBody>

          {serialPresetGroups && serialPresetGroups.length > 0 && (
            <div className="px-3 py-3 border-t border-divider">
              <SerialPresetControls
                presetGroups={serialPresetGroups}
                startStreaming={startStreaming}
                pauseStreaming={pauseStreaming}
                stopStreaming={stopStreaming}
              />
            </div>
          )}

          <CardFooter className="flex flex-row gap-3 items-center border-t border-divider">
            <span className="shrink-0">Grid Size:</span>
            <Input
              className="flex-1 min-w-0"
              type="number"
              value={gridSize.toString()}
              onValueChange={val => setGridSize(parseInt(val))}
            />
            <Button isIconOnly className="shrink-0" onPressStart={() => setGridSize(gridSize -1)}><Minus/></Button>
            <Button isIconOnly className="shrink-0" onPressStart={() => setGridSize(gridSize +1)}><Plus/></Button>
          </CardFooter>
        </Card>
      </div>
    </div>
  )
}