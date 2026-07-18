import React, { useState } from "react";
import { Button, Card, CardBody, CardHeader, Input, Tab, Tabs } from "@nextui-org/react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { ArrowClockwise, ArrowCounterclockwise } from "react-bootstrap-icons";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";

interface CacheConfig {
  label: string
  positionService: RosService
  twitchService: RosService
  storeKey: string
  reversed: boolean
}

const CachePositionNames = ["Closed", "Middle", "Open"]
const CachePositions = [152.47, 67.76, 0] // determined by physical testing


const CachePanel: React.FC<CacheConfig> = ({ label, positionService, twitchService, storeKey, reversed }) => {
  const [selected, setSelected] = useState<number>(0);
  const multiplier = reversed ? -1 : 1;

  const bifrostSetPos = useBifrost({ service: positionService });
  const bifrostTwitch = useBifrost({ service: twitchService });

  const [twitchStep, setTwitchStep] = useGenericStore<number>(storeKey + "TwitchStep");
  const [twitchInput, setTwitchInput] = useState((twitchStep ?? 5).toString());

  const setPosition = (key: number | string) => {
    bifrostSetPos.callService({ position: CachePositions[reversed ? CachePositions.length - 1 - Number(key) : Number(key)] });
    setSelected(Number(key));
  };

  const twitch = (step: number) => {
    bifrostTwitch.callService({ position: step });
  };

  return (
    <Card>
      <CardHeader>{`${label}`} cache</CardHeader>
      <CardBody className="flex flex-col pt-0">
        <div className="flex flex-col gap-3">
          <div className="flex flex-row">
            <Tabs key="tabs" color="primary" size="md" fullWidth selectedKey={selected.toString()} onSelectionChange={setPosition}>
              <Tab key={0} title={CachePositionNames[0]} />
              <Tab key={1} title={CachePositionNames[1]} />
              <Tab key={2} title={CachePositionNames[2]} />
            </Tabs>
          </div>
          <div className="flex flex-row justify-between gap-3">
            <Button onPress={() => twitch(-(twitchStep ?? 5) * multiplier)}><ArrowCounterclockwise /></Button>
            <Input
              type="number"
              value={twitchInput}
              onValueChange={(v) => { setTwitchInput(v); const n = parseFloat(v); if (!isNaN(n) && n > 0) setTwitchStep(n) }}
            />
            <Button onPress={() => twitch(twitchStep ?? 5 * multiplier)}><ArrowClockwise /></Button>
          </div>
        </div>
      </CardBody>
    </Card>

  );
};

const CacheControlWidget: React.FC = () => {
  return (
    <div className="flex flex-col gap-3">
      <div className="grid grid-cols-2 gap-3">
        <CachePanel
          label="Left"
          positionService={RosService.CACHE_LEFT_POSITION}
          twitchService={RosService.CACHE_LEFT_TWITCH}
          storeKey="cacheLeft"
          reversed={false}
        />
        <CachePanel
          label="Right"
          positionService={RosService.CACHE_RIGHT_POSITION}
          twitchService={RosService.CACHE_RIGHT_TWITCH}
          storeKey="cacheRight"
          reversed={true}
        />
      </div>
    </div >
  );
};

export default CacheControlWidget;
