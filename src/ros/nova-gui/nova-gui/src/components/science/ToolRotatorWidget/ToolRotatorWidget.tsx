import React, {useState} from "react";
import {Button, Card, CardBody, CardHeader, Input} from "@nextui-org/react";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {useToolRotatorServices} from "./useToolRotator.ts";

export interface PresetPositions {
  microscope: number
  nir_probe: number
  sweeper: number
}

const ToolRotatorWidget: React.FC = () => {
  const [setPreset, setPosition] = useToolRotatorServices()
  const [savedPresets, setSavedPresets] = useGenericStore<PresetPositions>("toolRotatorPresets")

  const [microscopeInput, setMicroscopeInput] = useState(savedPresets.microscope.toString())
  const [nirInput, setNirInput] = useState(savedPresets.nir_probe.toString())
  const [sweeperInput, setSweeperInput] = useState(savedPresets.sweeper.toString())

  const setPresetWrapped = (name: string, position: number) => {
    setSavedPresets({
      ...savedPresets,
      [name as keyof PresetPositions]: position
    } as PresetPositions)

    setPreset(name, position)
  }

  return (
    <Card>
      <CardHeader>
        Tool Rotator
      </CardHeader>

      <CardBody className="flex flex-col">
        <div className="grid grid-cols-3 gap-3">
          <div className="flex flex-col gap-3">
            <Input
              type="number"
              label="Sweeper"
              labelPlacement="outside"
              endContent={
                <div className="pointer-events-none flex items-center">
                  <span className="text-default-400 text-small">°</span>
                </div>
              }
              value={sweeperInput}
              onValueChange={setSweeperInput}
            />

            <div className="grid grid-cols-2 gap-3">
              <Button color="primary" onPressStart={() => setPresetWrapped("sweeper", parseFloat(sweeperInput))}>Set Preset</Button>
              <Button onPressStart={() => setPosition(parseFloat(sweeperInput))}>Go To</Button>
            </div>
          </div>
          <div className="flex flex-col gap-3">
            <Input
              type="number"
              label="Microscope"
              labelPlacement="outside"
              endContent={
                <div className="pointer-events-none flex items-center">
                  <span className="text-default-400 text-small">°</span>
                </div>
              }
              value={microscopeInput}
              onValueChange={setMicroscopeInput}
            />

            <div className="grid grid-cols-2 gap-3">
              <Button color="primary" onPressStart={() => setPresetWrapped("microscope", parseFloat(microscopeInput))}>Set Preset</Button>
              <Button onPressStart={() => setPosition(parseFloat(microscopeInput))}>Go To</Button>
            </div>
          </div>
          <div className="flex flex-col gap-3">
            <Input
              type="number"
              label="NIR Probe"
              labelPlacement="outside"
              endContent={
                <div className="pointer-events-none flex items-center">
                  <span className="text-default-400 text-small">°</span>
                </div>
              }
              value={nirInput}
              onValueChange={setNirInput}
            />

            <div className="grid grid-cols-2 gap-3">
              <Button color="primary" onPressStart={() => setPresetWrapped("nir_probe", parseFloat(nirInput))}>Set Preset</Button>
              <Button onPressStart={() => setPosition(parseFloat(nirInput))}>Go To</Button>
            </div>
          </div>
        </div>
      </CardBody>
    </Card>
  )
}

export default ToolRotatorWidget