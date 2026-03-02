// import {
//   Button,
//   Card,
//   CardBody,
//   CardHeader,
//   CardProps,
//   Chip,
//   Dropdown,
//   DropdownItem,
//   DropdownMenu,
//   DropdownTrigger,
//   Input,
//   Select,
//   SelectItem
// } from "@nextui-org/react";
// import CopyableOutput from "../../../shared/components/CopyableOutput/CopyableOutput.tsx";
// import React, {useCallback, useEffect, useMemo, useRef, useState} from "react";
// import {useBifrost} from "../../../../redux/actions/bifrost/useBifrostAction.ts";
// import {RosTopic} from "../../../../ros/topics/rosTopic.ts";
// import {useSelector} from "react-redux";
// import {RootState} from "../../../../redux/RootState.ts";
// import {Check, MoreHorizontal} from "react-feather";
// import {
//   ISpaceResourcesEntries,
//   ISpaceResourcesEntry,
//   NIRProbeReadingType,
//   NIRProbeReadingTypeInfo,
// } from "../SpaceResourcesSiteType.tsx";
// import {useNIRSiteData} from "../useNIRSiteData.ts";
// import {IRosScienceInterfacesNirProbeData, IRosScienceInterfacesNirProbeDataConst} from "../../../../ros/rosTypes.ts";
// import SpinnerButton from "../../../shared/components/buttons/SpinnerButton.tsx";

// export interface NIRProbeOutputSaveWidgetProps extends CardProps {
//   showAdvanced : boolean,
//   setShowAdvanced : (newShowAdvanced: boolean) => void,
//   readingInfo: NIRProbeReadingTypeInfo[], // list of NIRProbeReadingTypeInfo: [off, PD1, PD2]
// }

// /**
//  * Widget for displaying and saving data received from the NIR Probe
//  * @param showAdvanced
//  * @param setShowAdvanced
//  * @param cardProps
//  * @param readingInfo display information about each photodiode, should be of the form [off, PD1, PD2]
//  * @constructor
//  */
// const NIRProbeOutputSaveWidgetDupe: React.FC<NIRProbeOutputSaveWidgetProps> = ({
//   showAdvanced, setShowAdvanced, readingInfo, ...cardProps
// }) => {
//   const bifrost = useBifrost({ topic: RosTopic.NIR_DATA });
//   const [nirData, setNirData] = useState({
//     data: 0, led: 0, status: 1
//   } as IRosScienceInterfacesNirProbeData)
//   const [nirData2, setNirData2] = useState({
//     data: 0, led: 0, status: 1
//   } as IRosScienceInterfacesNirProbeData)
//   const [sampleLabel, setSampleLabel] = useState<string>("");

//   const [readings, setReadings] = useNIRSiteData();

//   const [data, setData] = useState<number | undefined>();
//   const [type, setType] = useState<NIRProbeReadingType.PD1 | NIRProbeReadingType.PD2>(NIRProbeReadingType.PD1);
//   const [advancedSampleLabel, setAdvancedSampleLabel] = useState<string>("");

//   const [autosave, setAutosave] = useState<boolean>(true)

//   const OffIcon = useMemo(() => readingInfo[0].icon, [readingInfo])
//   const PD1Icon = useMemo(() => readingInfo[1].icon, [readingInfo])
//   const PD2Icon = useMemo(() => readingInfo[2].icon, [readingInfo])
//   const icons = useMemo(() => [
//     <OffIcon size={18}/>,
//     <PD1Icon size={18}/>,
//     <PD2Icon size={18}/>,
//   ], [OffIcon, PD1Icon, PD2Icon])

//   // Used for autosaving
//   const previousDataRef = useRef<number | undefined>(undefined);
//   const previousDataRef2 = useRef<number | undefined>(undefined);
//   const [isLoading, setIsLoading] = useState(false)

//   useEffect(() => {
//     bifrost.syncWithTopic();
//   }, [bifrost]);

//   const onSave = useCallback(() => {
//     if (!showAdvanced && nirData.led === 0)
//       return

//     const saveType = showAdvanced && type ? type : nirData.led as keyof ISpaceResourcesEntries
//     setReadings({
//       ...readings,
//       [saveType]: [
//         {
//           data: showAdvanced && data ? data : nirData.data,
//           type: saveType,
//           label: showAdvanced ? advancedSampleLabel : sampleLabel,
//         } as ISpaceResourcesEntry,
//         ...readings[saveType],
//       ]
//     })
//   }, [readings, setReadings, data, type, sampleLabel, advancedSampleLabel, showAdvanced, nirData]);

//   const save = useCallback((reading: IRosScienceInterfacesNirProbeData) => {
//     if (!showAdvanced && reading.led === 0)
//       return

//     const saveType = showAdvanced && type ? type : nirData.led as keyof ISpaceResourcesEntries
//     const saveType2 = showAdvanced && type ? type : nirData2.led as keyof ISpaceResourcesEntries
//     setReadings({
//       ...readings,
//       [saveType]: [
//         {
//           data: nirData.data,
//           type: saveType,
//           label: "auto_" + (showAdvanced ? advancedSampleLabel : sampleLabel),
//         } as ISpaceResourcesEntry,
//         ...readings[saveType],
//       ],
//       [saveType2]: [
//         {
//           data: nirData2.data,
//           type: saveType2,
//           label: "auto_" + (showAdvanced ? advancedSampleLabel : sampleLabel),
//         } as ISpaceResourcesEntry,
//         ...readings[saveType2],
//       ]
//     })
//   }, [readings, setReadings, type, sampleLabel, advancedSampleLabel, showAdvanced, nirData]);

//   useEffect(() => {
//     if (!autosave)
//       return;

//     if (nirData.data === undefined)
//       return;

//     if (nirData.data === previousDataRef.current)
//       return;

//     previousDataRef.current = nirData.data;
//     save(nirData);
//   }, [autosave, save, nirData.data, previousDataRef]);

//   const onTypeChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
//     if (+e.target.value !== 0)
//       setType(+e.target.value as NIRProbeReadingType.PD1 | NIRProbeReadingType.PD2)
//   }

//   const takeFakeReadings = () => {
//     setNirData({
//       data: Math.floor(Math.random() * 553) + 35800, led: 1, status: 1
//     })

//     setNirData2({
//       data: Math.floor(Math.random() * 553) + 33800, led: 2, status: 1
//     })

//     setIsLoading(false)
//   }

//   const takeDelayedFakeReadings = () => {
//     setIsLoading(true)
//     setTimeout(takeFakeReadings, 500)
//   }

//   return (
//     <Card {...cardProps}>
//       <CardHeader className="pb-0 flex flex-row">
//         <div className="grow">NIR Probe Output</div>
//         <Dropdown className="m-0">
//           <DropdownTrigger>
//             <Button
//               variant={"light"}
//               isIconOnly
//               className="m-0"
//             >
//               <MoreHorizontal></MoreHorizontal>
//             </Button>
//           </DropdownTrigger>
//           <DropdownMenu aria-label="Static Actions">
//             <DropdownItem key="advanced" startContent={showAdvanced ? <Check/> : <></>}
//                           onPress={() => setShowAdvanced(!showAdvanced)}>
//               Show Advanced
//             </DropdownItem>
//             <DropdownItem key="autosave" startContent={autosave ? <Check/> : <></>}
//                           onPress={() => setAutosave(!autosave)}>
//               Autosave
//             </DropdownItem>
//           </DropdownMenu>
//         </Dropdown>
//       </CardHeader>
//       <CardBody className="flex flex-col gap-3">
//         <SpinnerButton
//           onPressStart={takeDelayedFakeReadings}
//           isLoading={isLoading}
//         >
//           Request LED Readings
//         </SpinnerButton>
//         <div className="flex flex-row gap-3 items-center">
//           <Chip size="lg"
//                 startContent={icons[nirData.led]}
//                 color={readingInfo[nirData.led].colour as "default" | "secondary" | "primary"}
//                 classNames={{
//                   base: "min-w-24",
//                 }}
//           >
//             {readingInfo[nirData.led].name}
//           </Chip>
//           <CopyableOutput className="tracking-wide grow" classNames={{pre: "text-lg pt-1"}}>
//             {nirData.data}
//           </CopyableOutput>
//           <Input onValueChange={setSampleLabel} value={sampleLabel} size="sm"
//                  labelPlacement="inside" label="Sample Label"
//                   className="w-1/4">
//           </Input>
//         </div>
//         <div className="flex flex-row gap-3 items-center">
//           <Chip size="lg"
//                 startContent={icons[nirData2.led]}
//                 color={readingInfo[nirData2.led].colour as "default" | "secondary" | "primary"}
//                 classNames={{
//                   base: "min-w-24",
//                 }}
//           >
//             {readingInfo[nirData2.led].name}
//           </Chip>
//           <CopyableOutput className="tracking-wide grow" classNames={{pre: "text-lg pt-1"}}>
//             {nirData2.data}
//           </CopyableOutput>
//           <Input onValueChange={setSampleLabel} value={sampleLabel} size="sm"
//                  labelPlacement="inside" label="Sample Label"
//                  className="w-1/4">
//           </Input>
//         </div>
//         <div className="grid auto-cols-fr gap-3 grid-flow-col">
//           {
//             <Button color="primary" onPress={onSave}>
//               Save Reading
//             </Button>
//           }
//         </div>
//       </CardBody>

//       {
//         showAdvanced &&
//         <CardBody className="flex flex-row gap-3">
//           <Input onValueChange={onFloatChanged(setData)} value={data?.toString() ?? ""} size="sm"
//             labelPlacement="outside" label={`Manual Reading Entry`}>
//           </Input>
//           <Select
//             selectedKeys={[`${type}`]}
//             size="sm"
//             labelPlacement="outside"
//             label="Reading Type"
//             onChange={onTypeChange}
//             aria-label="NIR Probe Type"
//             startContent={icons[type]}
//           >
//             {readingInfo.slice(1).map(({type, name}) => (
//               <SelectItem key={`${type}`} value={type} startContent={icons[type]}>
//                 {name}
//               </SelectItem>
//             ))}
//           </Select>
//           <Input onValueChange={setAdvancedSampleLabel} value={advancedSampleLabel} size="sm"
//             labelPlacement="outside" label="Sample Label">
//           </Input>
//         </CardBody>
//       }

//     </Card>
//   )
// }

// const onFloatChanged = (mutator: (x?: number) => void) => (userInput: string) => {
//   let parsedInput = userInput.length > 0 ? parseFloat(userInput) : undefined;
//   if (isNaN(parsedInput ?? 0))
//     parsedInput = undefined;

//   mutator(parsedInput);
// }

// export default NIRProbeOutputSaveWidgetDupe;