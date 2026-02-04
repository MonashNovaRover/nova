import ReactApexChart from "react-apexcharts";
import {ApexOptions} from "apexcharts";
import {Card, CardBody, CardHeader} from "@nextui-org/react";
import AnalysisArmDiagram from "./AnalysisPlatformDiagram.tsx";

export interface AnalysisArmWidgetProps {
}

/**
 * Diagram providing visualisation of the current state of the analysis arm.
 * @constructor
 */
const AnalysisArmWidget: React.FC<AnalysisArmWidgetProps> = () => {


  return (
    <Card>
      <CardHeader>
        Analysis Arm
      </CardHeader>
      <CardBody>
        <div className={"columns-2"}>
          <AnalysisArmDiagram percent={50} target={-15}/>
        </div>
      </CardBody>
    </Card>
  );
}

export default AnalysisArmWidget