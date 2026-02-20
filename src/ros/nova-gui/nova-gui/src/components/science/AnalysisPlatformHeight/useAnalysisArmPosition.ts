import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useEffect} from "react";

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