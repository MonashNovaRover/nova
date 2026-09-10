import {
  Button,
  Card,
  CardContent,
  CardHeader,
  CardProps,
  Chip,
  Dropdown,
  DropdownItem,
  DropdownMenu,
  DropdownTrigger,
  Input,
  ListBox,
  Select,
  ListBoxItem
} from "@heroui/react";
import CopyableOutput from "../../../shared/components/CopyableOutput/CopyableOutput.tsx";
import React, { useCallback, useEffect, useRef, useState } from "react";
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

  const onTypeChange = (key: React.Key | null) => {
    const selectedType = Number(key);
    if (key !== null && selectedType !== 0)
      setType(selectedType as NIRProbeReadingType.PD1 | NIRProbeReadingType.PD2)
  }

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0 flex flex-row">
        <div className="grow">NIR Probe Output</div>
        <Dropdown className="m-0">
          <DropdownTrigger>
            <Button
              variant="ghost"
              isIconOnly
              className="m-0"
            >
              <MoreHorizontal></MoreHorizontal>
            </Button>
          </DropdownTrigger>
          <Dropdown.Popover>
            <DropdownMenu aria-label="Static Actions">
              <DropdownItem id="advanced"
                onPress={() => setShowAdvanced(!showAdvanced)}>
                <span className="flex items-center gap-2">{showAdvanced && <Check />}Show Advanced</span>
              </DropdownItem>
              <DropdownItem id="autosave"
                onPress={() => setAutosave(!autosave)}>
                <span className="flex items-center gap-2">{autosave && <Check />}Autosave</span>
              </DropdownItem>
            </DropdownMenu>
          </Dropdown.Popover>
        </Dropdown>
      </CardHeader>
      <CardContent className="flex flex-col gap-3">
        <div className="flex flex-row gap-3 items-center">
          <Chip size="lg" variant="soft"
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
            color={readingInfo[nirData.reading_taken ? LED.nir1 : 0].colour === "primary" ? "accent" : "default"}
            className="min-w-24"
          >
            {readingInfo[nirData.reading_taken ? LED.nir1 : 0].name}
          </Chip>
          <CopyableOutput className="tracking-wide grow" classNames={{ pre: "text-lg pt-1" }}>
            {nirData.data[0]}
          </CopyableOutput>
          <Input aria-label="Sample Label" onChange={(event) => setSampleLabel(event.target.value)} value={sampleLabel} className="w-1/4" />
        </div>
        <div className="flex flex-row gap-3 items-center">
          <Chip size="lg"
            color={readingInfo[nirData.reading_taken ? LED.nir2 : 0].colour === "primary" ? "accent" : "default"}
            className="min-w-24"
          >
            {readingInfo[nirData.reading_taken ? LED.nir2 : 0].name}
          </Chip>
          <CopyableOutput className="tracking-wide grow" classNames={{ pre: "text-lg pt-1" }}>
            {nirData.data[1]}
          </CopyableOutput>
          <Input aria-label="Sample Label" onChange={(event) => setSampleLabel(event.target.value)} value={sampleLabel} className="w-1/4" />
        </div>
        <div className="grid auto-cols-fr gap-3 grid-flow-col">
          {
            <Button variant="primary" onPress={onSave}>
              Save Reading
            </Button>
          }
        </div>
      </CardContent>

      {
        showAdvanced &&
        <CardContent className="flex flex-row gap-3">
          <Input aria-label="Manual Reading Entry" onChange={(event) => onFloatChanged(setData)(event.target.value)} value={data?.toString() ?? ""} />
          <Select
            selectedKey={`${type}`}
            onSelectionChange={onTypeChange}
            aria-label="NIR Probe Type"
          >
            <Select.Trigger>
              <Select.Value />
              <Select.Indicator />
            </Select.Trigger>
            <Select.Popover>
              <ListBox>
                {readingInfo.slice(1).map(({ type, name }) => (
                  <ListBoxItem id={`${type}`} key={`${type}`}>
                    {name}
                  </ListBoxItem>
                ))}
              </ListBox>
            </Select.Popover>
          </Select>
          <Input aria-label="Sample Label" onChange={(event) => setAdvancedSampleLabel(event.target.value)} value={advancedSampleLabel} />
        </CardContent>
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