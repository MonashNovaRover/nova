import {
  Button,
  Card,
  CardBody,
  CardHeader,
  CardProps,
  Chip,
  Dropdown,
  DropdownItem,
  DropdownMenu,
  DropdownTrigger,
  Input,
  Select,
  SelectItem
} from "@nextui-org/react";
import CopyableOutput from "../../CopyableOutput/CopyableOutput.tsx";
import React, {useCallback, useEffect, useRef, useState} from "react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {Check, MoreHorizontal} from "react-feather";
import {
  ISpaceResourcesEntries,
  ISpaceResourcesEntry,
  NIRProbeReadingType,
  NIRPRobeReadingTypeInfo,
} from "../SpaceResourcesSiteType.tsx";
import {useNIRSiteData} from "../useNIRSiteData.ts";

export interface NIRProbeOutputSaveWidgetProps extends CardProps {
  showAdvanced : boolean,
  setShowAdvanced : (newShowAdvanced: boolean) => void,
}

/**
 * Widget for displaying and saving data received from the NIR Probe
 * @param showAdvanced
 * @param setShowAdvanced
 * @param cardProps
 * @constructor
 */
const NIRProbeOutputSaveWidget: React.FC<NIRProbeOutputSaveWidgetProps> = ({
  showAdvanced, setShowAdvanced, ...cardProps
}) => {
  const bifrost = useBifrost({ topic: RosTopic.NIR_DATA });
  const nirData = useSelector((state: RootState) => state.nirStore);
  const [sampleLabel, setSampleLabel] = useState<string>("");

  const [readings, setReadings] = useNIRSiteData();

  const [data, setData] = useState<number | undefined>();
  const [type, setType] = useState<NIRProbeReadingType.WATER | NIRProbeReadingType.ICE>(NIRProbeReadingType.WATER);
  const [advancedSampleLabel, setAdvancedSampleLabel] = useState<string>("");

  // Used for autosaving
  const previousDataRef = useRef<number | undefined>(undefined);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const onSave = useCallback(() => {
    if (!showAdvanced && nirData.led === 0)
      return

    const saveType = showAdvanced && type ? type : nirData.led as keyof ISpaceResourcesEntries
    setReadings({
      ...readings,
      [saveType]: [
        {
          data: showAdvanced && data ? data : nirData.data,
          type: saveType,
          label: showAdvanced ? advancedSampleLabel : sampleLabel,
        } as ISpaceResourcesEntry,
        ...readings[saveType],
      ]
    })
  }, [readings, setReadings, data, type, sampleLabel, advancedSampleLabel, showAdvanced, nirData]);

  const save = useCallback((reading: number) => {
    if (!showAdvanced && nirData.led === 0)
      return

    const saveType = showAdvanced && type ? type : nirData.led as keyof ISpaceResourcesEntries
    setReadings({
      ...readings,
      [saveType]: [
        {
          data: reading,
          type: saveType,
          label: showAdvanced ? advancedSampleLabel : sampleLabel,
        } as ISpaceResourcesEntry,
        ...readings[saveType],
      ]
    })
  }, [readings, setReadings, type, sampleLabel, advancedSampleLabel, showAdvanced, nirData]);

  useEffect(() => {
    if (data === undefined)
      return;

    if (data === previousDataRef.current)
      return;

    previousDataRef.current = data;
    save(data);
  }, [save, data, previousDataRef]);

  const onTypeChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    if (+e.target.value !== 0)
      setType(+e.target.value as NIRProbeReadingType.WATER | NIRProbeReadingType.ICE)
  }

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
        <div className="flex flex-row gap-3 items-center">
          <Chip size="lg"
                startContent={NIRPRobeReadingTypeInfo[nirData.led].icon}
                color={NIRPRobeReadingTypeInfo[nirData.led].colour as "default" | "secondary" | "primary"}
                classNames={{
                  base: "min-w-24",
                }}
          >
            {NIRPRobeReadingTypeInfo[nirData.led].name}
          </Chip>
          <CopyableOutput className="tracking-wide grow" classNames={{pre: "text-lg pt-1"}}>
            {nirData.data}
          </CopyableOutput>
          <Input onValueChange={setSampleLabel} value={sampleLabel} size="sm"
                 labelPlacement="inside" label="Sample Label"
                  className="w-1/4">
          </Input>
        </div>
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
          <Input onValueChange={onFloatChanged(setData)} value={data?.toString() ?? ""} size="sm"
            labelPlacement="outside" label={`Manual Reading Entry`}>
          </Input>
          <Select
            selectedKeys={[`${type}`]}
            size="sm"
            {...NIRPRobeReadingTypeInfo
              .slice(1)
              .map(({type, name, icon}) => (
              <SelectItem key={`${type}`} value={type} startContent={icon}>
                {name}
              </SelectItem>
            ))}

            labelPlacement="outside"
            label="Reading Type"
            onChange={onTypeChange}
            aria-label="NIR Probe Type"
            startContent={NIRPRobeReadingTypeInfo[type].icon}
          >
            {NIRPRobeReadingTypeInfo.slice(1).map(({type, name, icon}) => (
              <SelectItem key={`${type}`} value={type} startContent={icon}>
                {name}
              </SelectItem>
            ))}
          </Select>
          <Input onValueChange={setAdvancedSampleLabel} value={advancedSampleLabel} size="sm"
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