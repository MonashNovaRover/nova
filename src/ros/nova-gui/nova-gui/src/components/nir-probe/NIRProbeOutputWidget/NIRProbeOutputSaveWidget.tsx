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
  const led = useSelector((state: RootState) => state.nirStore.led);

  const [lightBlank, setLightBlank] = useState<number | undefined>();
  const [concentration, setConcentration] = useState<number | undefined>();
  const [manualReading, setManualReading] = useState<number | undefined>();
  const [sampleLabel, setSampleLabel] = useState<string>("");

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const updateLightBlank = useCallback(() => {
    setLightBlank(manualReading ?? nirData);
  }, [nirData, manualReading]);

  const updateDifference = useCallback(() => {
    const newEntry = {
      lightBlank: lightBlank,
      difference: (manualReading ?? nirData) - (lightBlank ?? 0),
      concentration: concentration,
      label: sampleLabel
    };
    const newFile: SiteData = { ...file, spaceResourcesEntries: [...file.spaceResourcesEntries, newEntry] };

    setFile(newFile);
  }, [file, nirData, manualReading, lightBlank, concentration, sampleLabel, setFile]);

  const onSave = useCallback(() => {
    if (led === 0)
      updateLightBlank();
    else
      updateDifference();
  }, [led, updateLightBlank, updateDifference]);

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
            showAdvanced ? (
              <>
                <Button color="default" onPress={updateLightBlank}>
                  Set Light Blank
                </Button>
                <Button color="primary" onPress={updateDifference}>
                  Save Reading
                </Button>
              </>
            ) : (
              <Button color={led === 0 ? "default" : "primary"} onPress={onSave}>
                {led === 0 ? "Set Light Blank" : "Save Reading"}
              </Button>
            )
          }
        </div>
      </CardBody>

      {
        showAdvanced &&
        <CardBody className="flex flex-row gap-3">
          <Input onValueChange={onFloatChanged(setConcentration)} value={concentration?.toString() ?? ""} size="sm"
                 labelPlacement="outside" label="Concentration">
          </Input>
          <Input onValueChange={onFloatChanged(setManualReading)} value={manualReading?.toString() ?? ""} size="sm"
                 labelPlacement="outside" label="Manual Reading Entry"
                 color={manualReading === undefined ? "default" : "danger"}>
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