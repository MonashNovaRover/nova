import React from "react";
import {Card, CardHeader, CardBody} from "@nextui-org/react";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";

interface PotentiostatWidgetProps {
  site: Site;
}

const PotentiostatWidget: React.FC<PotentiostatWidgetProps> = ({site}) => {
  return (
    <Card className="h-full">
      <CardHeader className="text-h1 pb-0">
        Site {site + 1} Potentiostat
      </CardHeader>
      <CardBody>
        {/* Empty for now */}
      </CardBody>
    </Card>
  );
};

export default PotentiostatWidget;
