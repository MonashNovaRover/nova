import {Card, CardBody, CardHeader, Switch} from "@nextui-org/react";
import AnalysisArmDiagram from "./AnalysisPlatformDiagram.tsx";
import AnalysisArmControls from "./AnalysisArmControls.tsx";
import {useState} from "react";
import {useAnalysisArmPosition} from "./useAnalysisArmPosition.ts";

export interface AnalysisArmWidgetProps {
}

/**
 * Diagram providing visualisation of the current state of the analysis arm.
 * @constructor
 */
const AnalysisArmWidget: React.FC<AnalysisArmWidgetProps> = () => {
  const [aaPos, tofDist] = useAnalysisArmPosition()

  const [useTOFData, setUseTOFData] = useState(false)

  return (
    <Card>
      <CardHeader>
        Analysis Arm
      </CardHeader>
      <CardBody>
        <div className="grid grid-cols-2">
          <div className="flex flex-col">
            <AnalysisArmDiagram percent={40} target={60} bottomDistance={tofDist.range} topDistance={convertStepsToDistance(aaPos.range)} topSteps={aaPos.range}/>

            <div className="flex flex-row justify-center gap-8">
              <p>Diagram uses:</p>
              <p>Steps</p>
              <Switch isSelected={useTOFData} onValueChange={setUseTOFData}/>
              <p>TOF</p>
            </div>
          </div>
          <AnalysisArmControls/>
        </div>
      </CardBody>
    </Card>
  );
}

const convertStepsToDistance = (steps: number) => steps / 2

export default AnalysisArmWidget