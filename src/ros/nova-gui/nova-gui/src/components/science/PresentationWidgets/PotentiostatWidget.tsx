import React, {useMemo} from "react";
import {Card, CardHeader, CardBody} from "@nextui-org/react";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import {usePotentiostatStorage} from "../Potentiostat/potentiostatStorage.ts";
import {PotentiostatChart} from "../Potentiostat/PotentiostatChart.tsx";

interface PotentiostatWidgetProps {
  site: Site;
}

const PotentiostatWidget: React.FC<PotentiostatWidgetProps> = ({site}) => {
  const {data} = usePotentiostatStorage();

  // Channel 1 = Site 1, Channel 2 = Site 2
  const channelData = useMemo(() => {
    if (site === Site.SITE_1) {
      return {channel1: data.channel1, channel2: []};
    } else if (site === Site.SITE_2) {
      return {channel1: [], channel2: data.channel2};
    }
    return null;
  }, [site, data.channel1, data.channel2]);

  // Sites 3 and 4 don't have potentiostat data
  if (channelData === null) {
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

  const relevantChannel = site === Site.SITE_1 ? data.channel1 : data.channel2;
  const hasData = relevantChannel.length > 0;

  return (
    <Card className="h-full">
      <CardHeader className="text-h1 pb-0">
        Site {site + 1} Potentiostat
      </CardHeader>
      <CardBody className="h-full flex items-center justify-center">
        {hasData ? (
          <PotentiostatChart
            channel1={channelData.channel1}
            channel2={channelData.channel2}
            calibration={data.calibration}
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
