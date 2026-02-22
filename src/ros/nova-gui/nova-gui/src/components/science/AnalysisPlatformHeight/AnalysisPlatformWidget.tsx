import {Card, CardBody, CardHeader} from "@nextui-org/react";
import AnalysisArmDiagram from "./AnalysisPlatformDiagram.tsx";
import {useAnalysisArmPosition, useAnalysisArmServices} from "./useAnalysisArmPosition.ts";
import {useCallback, useEffect, useMemo, useState} from "react";
import AnalysisArmControls from "./AnalysisArmControls.tsx";

export interface AnalysisArmWidgetProps {
}

/**
 * Diagram providing visualisation of the current state of the analysis arm.
 * @constructor
 */
const AnalysisArmWidget: React.FC<AnalysisArmWidgetProps> = () => {
  const [aaPos, tofDist] = useAnalysisArmPosition()
  const [zeroAA, setAAPos] = useAnalysisArmServices()

  // Target position
  const [target, setTarget] = useState(-100)

  const setPos = (newPos: number) => {
    setTarget(newPos)
    setAAPos(newPos)
  }

  // remove target market when the position is reached.
  useEffect(() => {
    if( aaPos.range === target)
      setTarget(-100)
  }, [aaPos.range, target]);

  // Converts a position (mm) to a percentage in the box chart.
  const convertToPercent = useCallback((num: number, curTofDist: number) => {
    // when aapos is less than min pin it to the top of the graph.
    if (aaPos.min_range > num) {
      return 100
    }

    // TOF distance is invalid outside its range.
    // When tof values are valid
    if (tofDist.min_range <= curTofDist <= tofDist.max_range) {
      const sum = num + curTofDist
      return 100 - (num / sum)
    }

    // when tof values are invalid (must not be close enough to the ground).
    // fall back to the aamax pos?
    return num / aaPos.max_range
  }, [aaPos.max_range, aaPos.min_range, tofDist.min_range, tofDist.max_range])

  const percent = useMemo(() => convertToPercent(aaPos.range, tofDist.range), [aaPos.range, tofDist.range])
  const targetPercent = useMemo(() => target < 0 ? -100 : convertToPercent(target, tofDist.range), [target, tofDist.range])

  return (
    <Card>
      <CardHeader>
        Analysis Arm
      </CardHeader>
      <CardBody>
        <div className="grid grid-cols-2">
          <div className="flex flex-col">
            <AnalysisArmDiagram percent={percent} target={targetPercent} bottomDistance={tofDist.range} topDistance={convertStepsToDistance(aaPos.range)}/>
          </div>
          <AnalysisArmControls currentPos={aaPos.range} TOFReading={tofDist.range} setPosition={setPos} zeroPosition={zeroAA}/>
        </div>
      </CardBody>
    </Card>
  );
}

const convertStepsToDistance = (steps: number) => steps / 2

export default AnalysisArmWidget