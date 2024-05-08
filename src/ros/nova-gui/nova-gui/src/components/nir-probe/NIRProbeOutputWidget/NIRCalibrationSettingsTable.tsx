import {NIRCalibrationData, NIRCalibrationPoint} from "./NIRCalibrationCurveWidget.tsx";
import {Button, Input, Table, TableBody, TableCell, TableColumn, TableHeader, TableRow} from "@nextui-org/react";
import React, {useCallback, useState} from "react";


export interface NIRCalibrationSettingsTableProps {
  data: NIRCalibrationData,
  setData: (newValue: NIRCalibrationData) => void
}

const formatPotentiallyNaNFloatString = (value: number) => isNaN(value) ? "" : `${value}`;

const NIRCalibrationSettingsTable : React.FC<NIRCalibrationSettingsTableProps> = ({
  data,
  setData
}) => {

  const [newDifference, setNewDifference] = useState<string>("");
  const [newConcentration, setNewConcentration] = useState<string>("");



  const addNewPoint = useCallback(() => {
    const newPoint: NIRCalibrationPoint = {
      difference: parseFloat(newDifference),
      concentration: parseFloat(newConcentration),
    }

    const newData = {
      ...data,
      points: [
        ...data.points,
        newPoint
      ]
    };

    setData(newData);

    setNewConcentration("");
    setNewDifference("");
  }, [data, setData, newDifference, setNewDifference, newConcentration, setNewConcentration]);

  const deletePoint = useCallback((index: number) => {
    const newData = {
      ...data,
      points: data.points.filter((_, i) => i !== index)
    };

    setData(newData);
  }, [data, setData]);

  const setYIntercept = useCallback((newYIntercept: number) => {
    setData({
      ...data,
      yIntercept: newYIntercept
    })
  }, [data, setData]);

  const setGradient = useCallback((newGradient: number) => {
    setData({
      ...data,
      gradient: newGradient
    })
  }, [data, setData]);

  const setChemBlankDifference = useCallback((newChemBlankDifference: number) => {
    setData({
      ...data,
      chemBlankDifference: newChemBlankDifference
    })
  }, [data, setData]);

  const dataEntryRow = (
    <TableRow key={"new-point-entry"}>
      <TableCell>
        <Input className="block" aria-label="new point difference" labelPlacement="outside-left"
               value={`${newDifference ?? ""}`}
               onValueChange={onFloatChanged(setNewDifference)}>
          hey
        </Input>
      </TableCell>
      <TableCell>
        <Input className="block" aria-label="new point concentration" labelPlacement="outside-left"
               value={`${newConcentration ?? ""}`}
               onValueChange={onFloatChanged(setNewConcentration)}>

        </Input>
      </TableCell>

      <TableCell>
        <Button onPress={addNewPoint}
                size="sm"
                color="primary"
                fullWidth
                isDisabled={newConcentration.length === 0 || newDifference.length === 0}>
          Add
        </Button>
      </TableCell>
    </TableRow>
  );

  return (<>
    <div className="flex flex-row gap-3">

      <Input label="Gradient" value={formatPotentiallyNaNFloatString(data.gradient)} onValueChange={v => setGradient(parseFloat(v))}/>
      <Input label="Y-Intercept" value={formatPotentiallyNaNFloatString(data.yIntercept)} onValueChange={v => setYIntercept(parseFloat(v))}/>
      <Input label="Chem Blank Difference" value={formatPotentiallyNaNFloatString(data.chemBlankDifference)} onValueChange={v => setChemBlankDifference(parseFloat(v))}/>
    </div>
    <Table aria-label="Example table with dynamic content">
      <TableHeader>
        <TableColumn key="difference">Difference</TableColumn>
        <TableColumn key="concentration">Concentration</TableColumn>
        <TableColumn key="concentration" className="text-center">Action</TableColumn>
      </TableHeader>
      <TableBody>
        {[
        ...data.points.map(({difference, concentration}, index) =>
            <TableRow key={index}>
              <TableCell>{difference}</TableCell>
              <TableCell>{concentration}</TableCell>

              <TableCell>
                <Button onPress={() => deletePoint(index)}
                        size="sm" color="danger" variant="light" className="block w-full">
                  Delete
                </Button>
              </TableCell>
            </TableRow>
          ),
          dataEntryRow
        ]}
      </TableBody>
    </Table>
  </>);
}

const onFloatChanged = (mutator: (x: string) => void) => (userInput: string) => {
  if (userInput.match(/((([1-9]([0-9]*))|0)(.?)([0-9]*))|(^$)/))
    mutator(userInput);
}

export default NIRCalibrationSettingsTable;