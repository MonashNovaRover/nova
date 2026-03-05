import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";

/**
 * Bifrost wrapper for calling Tool Rotator position services.
 */
export function useToolRotatorServices(): [(name: string, position: number) => void, (newPos: number) => void] {
  const bifrostPresets = useBifrost({service: RosService.TOOL_ROTATOR_PRESETS});
  const bifrostSetPos = useBifrost({service: RosService.TOOL_ROTATOR_POSITION});

  const setPresets = (name: string, position: number) => {
    bifrostPresets.callService({names: [name], positions: [position]})
  }

  const setPos = (newPos: number) => {
    bifrostSetPos.callService({positions: [newPos]})
  }

  return [setPresets, setPos]
}
