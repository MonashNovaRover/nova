import {
  Button,
  Card,
  CardBody,
  CardHeader,
  CardProps,
  Dropdown, DropdownItem,
  DropdownMenu,
  DropdownTrigger,
  Input
} from "@nextui-org/react";
import CopyableOutput from "../../CopyableOutput/CopyableOutput.tsx";
import React, {useCallback, useEffect, useState} from "react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {Check, MoreHorizontal} from "react-feather";
import {SiteData} from "../../../redux/models/genericStores/SiteDataState.ts";
import {ISpaceResourcesEntry, XYNames} from "../SpaceResourcesSiteType.tsx";
import {useNIRSiteData} from "../useNIRSiteData.ts";

export interface NIRProbeOutputSaveWidgetProps extends CardProps {
  file: SiteData,
  setFile: (newFile: SiteData) => void,
  showAdvanced : boolean,
  setShowAdvanced : (newShowAdvanced: boolean) => void,
}

const NIRProbeOutputSaveWidget: React.FC<NIRProbeOutputSaveWidgetProps> = ({
  file, setFile, showAdvanced, setShowAdvanced, ...cardProps
}) => {
  const bifrost = useBifrost({ topic: RosTopic.NIR_DATA });
  const nirData = useSelector((state: RootState) => state.nirStore.data);
  // const led = useSelector((state: RootState) => state.nirStore.led);

  const [readings, setReadings] = useNIRSiteData();

  const [x, setX] = useState<number | undefined>();
  const [y, setY] = useState<number | undefined>();
  const [sampleLabel, setSampleLabel] = useState<string>("");

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const onSave = useCallback(() => {
    setReadings([
      ...readings,
      {
        x: x,
        y: y,
        label: sampleLabel,
      } as ISpaceResourcesEntry
    ])
  }, [readings, setReadings, x, y, sampleLabel]);

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0 flex flex-row">
        <div className="grow">NIR Probe Output</div>
        <Dropdown className="m-0">
          <DropdownTrigger>
            <Button
              variant={"light"}
              isIconOnly
              className="m-0"
            >
              <MoreHorizontal></MoreHorizontal>
            </Button>
          </DropdownTrigger>
          <DropdownMenu aria-label="Static Actions">
            <DropdownItem key="advanced" startContent={showAdvanced ? <Check/> : <></>}
                          onPress={() => setShowAdvanced(!showAdvanced)}>
              Show Advanced
            </DropdownItem>
          </DropdownMenu>
        </Dropdown>
      </CardHeader>
      <CardBody className="flex flex-col gap-3">
        <CopyableOutput className="tracking-wide" classNames={{pre: "text-lg pt-1"}}>
          {nirData}
        </CopyableOutput>
        <div className="grid auto-cols-fr gap-3 grid-flow-col">
          {
            <Button color="primary" onPress={onSave}>
              Save Reading
            </Button>
          }
        </div>
      </CardBody>

      {
        showAdvanced &&
        <CardBody className="flex flex-row gap-3">
          <Input onValueChange={onFloatChanged(setX)} value={x?.toString() ?? ""} size="sm"
                 labelPlacement="outside" label={`Manual ${XYNames.Y} Reading Entry`}>
          </Input>
          <Input onValueChange={onFloatChanged(setY)} value={y?.toString() ?? ""} size="sm"
                 labelPlacement="outside" label={`Manual ${XYNames.Y} Reading Entry`}>
          </Input>
          <Input onValueChange={setSampleLabel} value={sampleLabel} size="sm"
                 labelPlacement="outside" label="Sample Label">
          </Input>
        </CardBody>
      }

    </Card>
  )
}

const onFloatChanged = (mutator: (x?: number) => void) => (userInput: string) => {
  let parsedInput = userInput.length > 0 ? parseFloat(userInput) : undefined;
  if (isNaN(parsedInput ?? 0))
    parsedInput = undefined;

  mutator(parsedInput);
}

export default NIRProbeOutputSaveWidget;