import React from "react";
import {Card, CardHeader, CardBody} from "@nextui-org/react";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {usePotentiostatStorage} from "../Potentiostat/potentiostatStorage.ts";
import {PotentiostatChart} from "../Potentiostat/PotentiostatChart.tsx";

interface PotentiostatWidgetProps {
  site: Site;
}

const PotentiostatWidget: React.FC<PotentiostatWidgetProps> = ({site}) => {
  const {data} = usePotentiostatStorage();

  // Sites 3 and 4 don't have potentiostat data
  if (site !== Site.SITE_1 && site !== Site.SITE_2) {
    return (
      <Card className="h-full">
        <CardHeader className="text-h1 pb-0">
          Site {site + 1} Potentiostat
        </CardHeader>
        <CardBody className="h-full flex items-center justify-center">
          <p className="text-default-500">No potentiostat available for this site</p>
        </CardBody>
      </Card>
    );
  }

  const hasData = data.channel1.length > 0 || data.channel2.length > 0;

  return (
    <Card className="h-full">
      <CardHeader className="text-h1 pb-0">
        Site {site + 1} Potentiostat
      </CardHeader>
      <CardBody className="h-full flex items-center justify-center">
        {hasData ? (
          <PotentiostatChart
            channel1={data.channel1}
            channel2={data.channel2}
            mode="measurement"
            height={300}
          />
        ) : (
          <p className="text-default-500">No measurements recorded</p>
        )}
      </CardBody>
    </Card>
  );
};

export default PotentiostatWidget;
