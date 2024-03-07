import {
  Button,
  Card,
  CardBody,
  CardHeader,
  CardProps, Divider, Input,
  Table,
  TableBody, TableCell,
  TableColumn,
  TableHeader, TableRow,
} from "@nextui-org/react";
import React, {useCallback, useEffect, useState} from "react";
import CopyableOutput from "../../CopyableOutput/CopyableOutput.tsx";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";


interface INIRProbeValueWidgetProps extends CardProps {

}


interface ISpaceResourcesEntry {
  lightBlank?: number,
  difference: number,
  concentration?: number,
  label: string
}

interface ISpaceResourcesFile {
  entries: ISpaceResourcesEntry[]
}

const LOCAL_STORAGE_KEY = "space-resources-test2";


const NIRProbeValueWidget: React.FC<INIRProbeValueWidgetProps> = ({...cardProps}) => {
  const bifrost = useBifrost({ topic: RosTopic.NIR_DATA });
  const nirData = useSelector((state: RootState) => state.nirStore.data);
  //const led = useSelector((state: RootState) => state.nirStore.led);

  const [file, setFile] = useState<ISpaceResourcesFile>({entries: []})
  const [lightBlank, setLightBlank] = useState<number | undefined>();
  const [concentration, setConcentration] = useState<number | undefined>();
  const [manualReading, setManualReading] = useState<number | undefined>();
  const [sampleLabel, setSampleLabel] = useState<string>("");

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  useEffect(() => {
    const storedFile = localStorage.getItem(LOCAL_STORAGE_KEY);
    if (storedFile === null)
      return;

    setFile(JSON.parse(storedFile));
  }, []);

  const updateLightBlank = useCallback(() => {
    setLightBlank(nirData);
  }, [nirData]);

  const updateDifference = useCallback(() => {
    const newEntry = {
      lightBlank: lightBlank,
      difference: (nirData) - (lightBlank ?? 0),
      concentration: concentration,
      label: sampleLabel
    };
    const newFile = { entries: [...file.entries, newEntry] };

    localStorage.setItem(LOCAL_STORAGE_KEY, JSON.stringify(newFile));
    setFile(newFile);

  }, [file, nirData, manualReading, lightBlank, concentration, sampleLabel]);

  const deleteEntry = useCallback((index: number) => {
    const newFile = {
      entries: file.entries.filter((_, i) => i !== index)
    };

    localStorage.setItem(LOCAL_STORAGE_KEY, JSON.stringify(newFile));
    setFile(newFile);
  }, [file]);

  /*const onSave = useCallback(() => {
    if (led === 0) {
      updateLightBlank();
      return;
    }

    updateDifference();
  }, [led, updateLightBlank, updateDifference]);*/

  const onConcentrationChanged = (userInput: string) => {
    let input = userInput.length > 0 ? parseFloat(userInput) : undefined;
    if (isNaN(input ?? 0))
      input = undefined;

    setConcentration(input);
  }

  const onManualReadingChanged = (userInput: string) => {
    let input = userInput.length > 0 ? parseFloat(userInput) : undefined;
    if (isNaN(input ?? 0))
      input = undefined;

    setManualReading(input);
  }

  /*
  <Button color={led === 0 ? "default" : "primary"} onClick={onSave}>
    {led === 0 ? "Set Light Blank" : "Save Reading"}
  </Button>
   */

  let reversedEntries = [...file.entries];
  reversedEntries.reverse();

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe Output
      </CardHeader>
      <CardBody className="flex flex-col gap-3">
        <CopyableOutput className="tracking-wide" classNames={{pre: "text-lg pt-1"}}>
          {nirData}
        </CopyableOutput>
        <div className="grid auto-cols-fr grid-cols-2 gap-3">
          <Button color="default" onClick={updateLightBlank}>
            Set Light Blank
          </Button>
          <Button color="primary" onClick={updateDifference}>
            Save
          </Button>
        </div>

      </CardBody>

      <Divider/>

      <CardBody className="flex flex-row gap-3">
        <Input onValueChange={onConcentrationChanged} value={concentration?.toString() ?? ""} size="sm"
               labelPlacement="outside" label="Concentration">
        </Input>
        <Input onValueChange={onManualReadingChanged} value={manualReading?.toString() ?? ""} size="sm"
               labelPlacement="outside" label="Manual Reading Entry">
        </Input>
        <Input onValueChange={setSampleLabel} value={sampleLabel} size="sm"
               labelPlacement="outside" label="Sample Label">
        </Input>
      </CardBody>

      <Divider/>

      <CardBody className="flex flex-col gap-3">
        <Table aria-label="Example table with dynamic content">
          <TableHeader>
            <TableColumn key="lightBlank">Light Blank</TableColumn>
            <TableColumn key="difference">Difference</TableColumn>
            <TableColumn key="concentration">Concentration</TableColumn>
            <TableColumn key="label">Label</TableColumn>
            <TableColumn>Difference</TableColumn>
          </TableHeader>
          <TableBody>
            {file.entries.map(({lightBlank, difference, concentration, label}, index) =>
              <TableRow key={index}>
                <TableCell>
                  { lightBlank ?
                    <span className="text-gray-500">{lightBlank}</span> :
                    <span className="text-gray-800">{lightBlank ?? "None"}</span>
                  }
                </TableCell>
                <TableCell>{difference}</TableCell>
                <TableCell>
                  { concentration !== undefined ?
                    <span className="text-gray-500">{concentration}</span> :
                    <span className="text-gray-800">None</span>
                  }
                </TableCell>
                <TableCell>
                  { 
                    label 
                  }
                </TableCell>
                <TableCell>
                  <Button onClick={() => deleteEntry(index)} size="sm" color="danger">
                    Delete
                  </Button>
                </TableCell>
              </TableRow>
            )}
          </TableBody>
        </Table>
      </CardBody>

      <Divider></Divider>

      <CardBody>
        {file.entries.map(({lightBlank, difference, concentration, label}, index) => 
          <>{`${lightBlank},${difference},${concentration},${label}\n`}<br/></>
        )}

      </CardBody>

    </Card>
  );
}

export default NIRProbeValueWidget;

