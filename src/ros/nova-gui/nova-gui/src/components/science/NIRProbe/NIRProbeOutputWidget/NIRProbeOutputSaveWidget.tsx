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
import CopyableOutput from "../../../shared/components/CopyableOutput/CopyableOutput.tsx";
import React, { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../../ros/topics/rosTopic.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import { Check, MoreHorizontal } from "react-feather";
import {
  ISpaceResourcesEntry,
  NIRProbeReadingType,
  NIRProbeReadingTypeInfo,
} from "../SpaceResourcesSiteType.tsx";
import { useNIRSiteData } from "../useNIRSiteData.ts";
import { IRosScienceInterfacesNirProbeData } from "../../../../ros/rosTypes.ts";
import { RosService } from "../../../../ros/services/rosService.ts";
import { isEqual } from "lodash";

export interface NIRProbeOutputSaveWidgetProps extends CardProps {
  showAdvanced: boolean,
  setShowAdvanced: (newShowAdvanced: boolean) => void,
  readingInfo: NIRProbeReadingTypeInfo[], // list of NIRProbeReadingTypeInfo: [off, PD1, PD2]
}

/**
 * Constants for NIR Leds
 * 
 */
const LED = {
  nir1: 1,
  nir2: 2,
  off: 0
}

/**
 * Widget for displaying and saving data received from the NIR Probe
 * @param showAdvanced
 * @param setShowAdvanced
 * @param cardProps
 * @param readingInfo display information about each photodiode, should be of the form [off, PD1, PD2]
 * @constructor
 */
const NIRProbeOutputSaveWidget: React.FC<NIRProbeOutputSaveWidgetProps> = ({
  showAdvanced, setShowAdvanced, readingInfo, ...cardProps
}) => {
  const bifrost = useBifrost({ topic: RosTopic.NIR_DATA, service: RosService.TAKE_NIR_PROBE_READING });
  const nirData = useSelector((state: RootState) => state.nirStore);
  const takeReading = () => bifrost.callService({});
  const [sampleLabel, setSampleLabel] = useState<string>("");

  const [readings, setReadings] = useNIRSiteData();

  const [data, setData] = useState<number | undefined>();
  const [type, setType] = useState<NIRProbeReadingType.PD1 | NIRProbeReadingType.PD2>(NIRProbeReadingType.PD1);
  const [advancedSampleLabel, setAdvancedSampleLabel] = useState<string>("");

  const [autosave, setAutosave] = useState<boolean>(true)

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const OffIcon = useMemo(() => readingInfo[0].icon, [readingInfo])
  const PD1Icon = useMemo(() => readingInfo[1].icon, [readingInfo])
  const PD2Icon = useMemo(() => readingInfo[2].icon, [readingInfo])
  const icons = useMemo(() => [
    <OffIcon size={18} />,
    <PD1Icon size={18} />,
    <PD2Icon size={18} />,
  ], [OffIcon, PD1Icon, PD2Icon])


  // Used for autosaving
  const previousDataRef = useRef<number[] | undefined>(undefined);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const onSave = useCallback(() => {
    if (!nirData.data || nirData.data.length < 2) return;

    setReadings({
      ...readings,
      [NIRProbeReadingType.PD1]: [
        {
          data: showAdvanced && data ? data : nirData.data[0],
          type: NIRProbeReadingType.PD1,
          label: showAdvanced ? advancedSampleLabel : sampleLabel,
        },
        ...readings[NIRProbeReadingType.PD1],
      ],
      [NIRProbeReadingType.PD2]: [
        {
          data: showAdvanced && data ? data : nirData.data[1],
          type: NIRProbeReadingType.PD2,
          label: showAdvanced ? advancedSampleLabel : sampleLabel,
        },
        ...readings[NIRProbeReadingType.PD2],
      ]
    });
  }, [readings, setReadings, data, sampleLabel, advancedSampleLabel, showAdvanced, nirData]);

  const save = useCallback((reading: IRosScienceInterfacesNirProbeData) => {
    if (!showAdvanced && !nirData.status)
      return


    setReadings({
      ...readings,
      [NIRProbeReadingType.PD1]: [
        {
          data: reading.data[0],
          type: NIRProbeReadingType.PD1,
          label: "auto_" + (showAdvanced ? advancedSampleLabel : sampleLabel),
        } as ISpaceResourcesEntry,
        ...readings[NIRProbeReadingType.PD1],
      ],
      [NIRProbeReadingType.PD2]: [
        {
          data: reading.data[1],
          type: NIRProbeReadingType.PD2,
          label: "auto_" + (showAdvanced ? advancedSampleLabel : sampleLabel),
        } as ISpaceResourcesEntry,
        ...readings[NIRProbeReadingType.PD2],
      ]
    })
  }, [readings, setReadings, sampleLabel, advancedSampleLabel, showAdvanced, nirData]);

  useEffect(() => {
    if (!autosave)
      return;

    if (nirData.data === undefined)
      return;

    if (isEqual(nirData.data, previousDataRef.current))
      return;

    previousDataRef.current = [...nirData.data];;
    save(nirData);
  }, [autosave, save, nirData, previousDataRef]);

  const onTypeChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    if (+e.target.value !== 0)
      setType(+e.target.value as NIRProbeReadingType.PD1 | NIRProbeReadingType.PD2)
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
            <DropdownItem key="advanced" startContent={showAdvanced ? <Check /> : <></>}
              onPress={() => setShowAdvanced(!showAdvanced)}>
              Show Advanced
            </DropdownItem>
            <DropdownItem key="autosave" startContent={autosave ? <Check /> : <></>}
              onPress={() => setAutosave(!autosave)}>
              Autosave
            </DropdownItem>
          </DropdownMenu>
        </Dropdown>
      </CardHeader>
      <CardBody className="flex flex-col gap-3">
        <div className="flex flex-row gap-3 items-center">
          <Chip radius="md" size="lg" variant="dot"
            color={nirData.status ? "warning" : "success"}
            className={`h-10 border-2 ${nirData.status ? "border-warning" : "border-success"}`}
          >
            {nirData.status ? "Busy" : "Idle"}
          </Chip>
          <Button fullWidth onPress={() => takeReading()}>
            Request LED Readings
          </Button>
        </div>
        <div className="flex flex-row gap-3 items-center">
          <Chip size="lg"
            startContent={icons[nirData.reading_taken ? LED.nir1 : 0]}
            color={readingInfo[nirData.reading_taken ? LED.nir1 : 0].colour as "default" | "secondary" | "primary"}
            classNames={{
              base: "min-w-24",
            }}
          >
            {readingInfo[nirData.reading_taken ? LED.nir1 : 0].name}
          </Chip>
          <CopyableOutput className="tracking-wide grow" classNames={{ pre: "text-lg pt-1" }}>
            {nirData.data[0]}
          </CopyableOutput>
          <Input onValueChange={setSampleLabel} value={sampleLabel} size="sm"
            labelPlacement="inside" label="Sample Label"
            className="w-1/4">
          </Input>
        </div>
        <div className="flex flex-row gap-3 items-center">
          <Chip size="lg"
            startContent={icons[nirData.reading_taken ? LED.nir2 : 0]}
            color={readingInfo[nirData.reading_taken ? LED.nir2 : 0].colour as "default" | "secondary" | "primary"}
            classNames={{
              base: "min-w-24",
            }}
          >
            {readingInfo[nirData.reading_taken ? LED.nir2 : 0].name}
          </Chip>
          <CopyableOutput className="tracking-wide grow" classNames={{ pre: "text-lg pt-1" }}>
            {nirData.data[1]}
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
            labelPlacement="outside"
            label="Reading Type"
            onChange={onTypeChange}
            aria-label="NIR Probe Type"
            startContent={icons[type]}
          >
            {readingInfo.slice(1).map(({ type, name }) => (
              <SelectItem key={`${type}`} value={type} startContent={icons[type]}>
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