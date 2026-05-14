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
}

const CachePanel: React.FC<CacheConfig> = ({ positionService, twitchService, storeKey }) => {
  const bifrostSetPos = useBifrost({ service: positionService });
  const bifrostTwitch = useBifrost({ service: twitchService });

  const [twitchStep, setTwitchStep] = useGenericStore<number>(storeKey + "TwitchStep");
  const [twitchInput, setTwitchInput] = useState((twitchStep ?? 5).toString());

  const setPosition = (angle: number) => {
    bifrostSetPos.callService({ position: angle });
  };

  const twitch = (step: number) => {
    bifrostTwitch.callService({ position: step });
  };

  return (
    <div className="flex flex-col gap-3">
      <div className="grid grid-cols-3 gap-3">
        <Button color="primary" onPress={() => setPosition(0)}>Closed</Button>
        <Button color="primary" onPress={() => setPosition(90)}>Half Open</Button>
        <Button color="primary" onPress={() => setPosition(180)}>Fully Open</Button>
      </div>
      <div className="grid grid-cols-3 gap-3">
        <Button onPress={() => twitch(-(twitchStep ?? 5))}><ArrowCounterclockwise size={18} /></Button>
        <Input
          type="number"
          label="Twitch"
          size="sm"
          endContent={
            <div className="pointer-events-none flex items-center">
              <span className="text-default-400 text-small">°</span>
            </div>
          }
          value={twitchInput}
          onValueChange={(v) => { setTwitchInput(v); const n = parseFloat(v); if (!isNaN(n) && n > 0) setTwitchStep(n) }}
        />
        <Button onPress={() => twitch(twitchStep ?? 5)}><ArrowClockwise size={18} /></Button>
      </div>
    </div>
  );
};

const CacheControlWidget: React.FC = () => {
  return (
    <Card>
      <CardHeader>Caches</CardHeader>
      <CardBody className="pt-0">
        <Tabs fullWidth>
          <Tab key="left" title="Left">
            <CachePanel
              label="Left"
              positionService={RosService.CACHE_LEFT_POSITION}
              twitchService={RosService.CACHE_LEFT_TWITCH}
              storeKey="cacheLeft"
            />
          </Tab>
          <Tab key="right" title="Right">
            <CachePanel
              label="Right"
              positionService={RosService.CACHE_RIGHT_POSITION}
              twitchService={RosService.CACHE_RIGHT_TWITCH}
              storeKey="cacheRight"
            />
          </Tab>
        </Tabs>
      </CardBody>
    </Card>
  );
};

export default CacheControlWidget;
