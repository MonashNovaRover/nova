import {useEffect} from "react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {PresetPositions} from "./ToolRotatorWidget.tsx";
/**
 * Bifrost wrapper for calling Tool Rotator position services.
 */
export function useToolRotatorServices(): [(name: string, position: number) => void, (newPos: number) => void, (step: number) => void] {
  const bifrostPresets = useBifrost({service: RosService.TOOL_ROTATOR_PRESETS});
  const bifrostSetPos = useBifrost({service: RosService.TOOL_ROTATOR_POSITION});
  const bifrostTwitch = useBifrost({service: RosService.TOOL_ROTATOR_TWITCH});
  const rosAngleBifrost = useBifrost({topic: RosTopic.TOOL_ROTATOR_ANGLE})
  
  useEffect(() => { 
    rosAngleBifrost.syncWithTopic() 
  }, [rosAngleBifrost])

  const setPresets = (name: string, position: number) => {
    bifrostPresets.callService({names: [name], positions: [position]})
  }
  
  const setPos = (newPos: number) => {
    bifrostSetPos.callService({position: newPos})
  }

  const twitchPos = (step: number) => {
    bifrostTwitch.callService({ position: step }, { responseToast: false })
  }
  return [setPresets, setPos, twitchPos]
}

/**
 * Global keyboard shortcuts for Tool Rotator controls.
 */
export function useToolRotatorKeyboard() {
  const [, setPosition, twitchPos] = useToolRotatorServices()
  const [toolRotatorPresets] = useGenericStore<PresetPositions>("toolRotatorPresets")
  const [twitchStep] = useGenericStore<number>("toolRotatorTwitchStep")
  
  useEffect(() => {
    const handleKey = (e: KeyboardEvent) => {
      switch (e.key.toLowerCase()) {
        case 'q': twitchPos(-twitchStep); break;
        case 'e': twitchPos(twitchStep); break;
        case 'j': setPosition(toolRotatorPresets.sweeper); break;
        case 'k': setPosition(toolRotatorPresets.microscope); break;
        case 'l': setPosition(toolRotatorPresets.nir_probe); break;
      }
    }
    window.addEventListener('keyup', handleKey);
    return () => window.removeEventListener('keyup', handleKey);
  }, [twitchStep, twitchPos, toolRotatorPresets, setPosition])
}