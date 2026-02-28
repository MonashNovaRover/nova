import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useEffect} from "react";
import {RosService} from "../../../ros/services/rosService.ts";

/**
 * Bifrost wrapper for analysis arm and tof published data.
 */
export function useAnalysisArmPosition() {
  const tofStore = useSelector(
    (state: RootState) => state.tofStore
  );

  const aaPosStore = useSelector(
    (state: RootState) => state.aaPosStore
  );

  const bifrostTOF = useBifrost({ topic: RosTopic.TOF });
  const bifrostAAPOS = useBifrost({ topic: RosTopic.AA_POS });

  useEffect(() => {
    bifrostTOF.syncWithTopic();
    bifrostAAPOS.syncWithTopic();
  }, [bifrostTOF, bifrostAAPOS]);

  return [aaPosStore, tofStore]
}

/**
 * Bifrost wrapper for calling analysis arm position services.
 */
export function useAnalysisArmServices(): [() => void, (newPos: number) => void, () => void, () => void] {
  const bifrostZero = useBifrost({service: RosService.ZERO_ANALYSIS_ARM});
  const bifrostSetPos = useBifrost({service: RosService.SET_AA_POSITION});
  const bifrostStopAA = useBifrost({service: RosService.STOP_AA_MOVEMENT});
  const birfrostTofReset = useBifrost({service: RosService.RESET_TOF});

  const zeroAA = () => {
    bifrostZero.callService({})
  }

  const setPos = (newPos: number) => {
    bifrostSetPos.callService({position: newPos})
  }

  const stopAA = () => {
    bifrostStopAA.callService({})
  }

  const resetTOF = () => {
    birfrostTofReset.callService({})
  }

  // const toggleActiveStatus = (value: boolean) => {
  //   bifrost.callService({state: value}, {
  //     handleResponse: (response) => {
  //       const boolResponse = response as IRosScienceInterfacesKilnCommandResponse;
  //       if (boolResponse?.success) {
  //         toast.success("Request Successful")
  //         setStepperActive(value);
  //       }
  //     }
  //   });
  // }

  return [zeroAA, setPos, stopAA, resetTOF]
}