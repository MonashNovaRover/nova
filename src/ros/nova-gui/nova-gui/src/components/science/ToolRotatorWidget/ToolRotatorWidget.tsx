import React, { useState } from "react";
import { Button, Card, CardBody, CardHeader, Input } from "@nextui-org/react";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";
import { useToolRotatorKeyboard, useToolRotatorServices } from "./useToolRotator.ts";
import { ArrowClockwise, ArrowCounterclockwise } from "react-bootstrap-icons";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";

export interface PresetPositions {
  microscope: number
  nir_probe: number
  sweeper: number
}

/**
 * Widget to set ToolRotator preset positions and set the current position.
 * @constructor
 */
const ToolRotatorWidget: React.FC = () => {
  const [setPreset, setPosition, twitchPos] = useToolRotatorServices()
  const [savedPresets, setSavedPresets] = useGenericStore<PresetPositions>("toolRotatorPresets")
  const currentAngle = useSelector((state: RootState) => state.toolRotatorAngleStore.data)

  const [microscopeInput, setMicroscopeInput] = useState(savedPresets.microscope.toString())
  const [nirInput, setNirInput] = useState(savedPresets.nir_probe.toString())
  const [sweeperInput, setSweeperInput] = useState(savedPresets.sweeper.toString())
  const [twitchStep, setTwitchStep] = useGenericStore<number>("toolRotatorTwitchStep")
  const [twitchInput, setTwitchInput] = useState(twitchStep.toString())

  const setPresetWrapped = (name: string, position: number) => {
    setSavedPresets({
      ...savedPresets,
      [name as keyof PresetPositions]: position
    } as PresetPositions)

    setPreset(name, position)
  }

  const twitch = (step: number) => {
    twitchPos(step)
  }

  useToolRotatorKeyboard()

  return (
    <Card>
      <CardHeader>
        Tool Rotator
      </CardHeader>

      <CardBody className="flex flex-col pt-0">
        <div className="grid grid-cols-4 gap-3">
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
          <div className="flex flex-col gap-3">
            <div className="grid grid-cols-2 gap-2">
              <Input
                type="number"
                className="h-10 flex-1"
                label="Twitch"
                labelPlacement="outside"
                endContent={
                  <div className="pointer-events-none flex items-center">
                    <span className="text-default-400 text-small">°</span>
                  </div>
                }
                value={twitchInput}
                onValueChange={(v) => { setTwitchInput(v); const n = parseFloat(v); if (!isNaN(n)) setTwitchStep(n) }}
              />
              <div className="self-end grid place-items-center h-10 rounded-xl border-2 border-primary text-primary text-white">
                {(currentAngle ?? 120).toFixed(2)}°
              </div>
            </div>
            <div className="grid grid-cols-2 gap-3">
              <Button isDisabled={currentAngle <= 0} onPressStart={() => twitch(-twitchStep)}><ArrowCounterclockwise size={20} /></Button>
              <Button isDisabled={currentAngle >= 360} onPressStart={() => twitch(twitchStep)}><ArrowClockwise size={20} /></Button>
            </div>
          </div>
        </div>
      </CardBody>
    </Card>
  )
}

export default ToolRotatorWidget