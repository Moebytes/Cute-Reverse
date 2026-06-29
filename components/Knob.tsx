import React, {useId} from "react"
import {KnobHeadless, KnobHeadlessLabel, KnobHeadlessOutput, useKnobKeyboardControls} from "react-knob-headless"
import withJuceSlider, {WithJUCESliderProps} from "./withJuceSlider"
import functions from "../structures/Functions"
import "./styles/knob.scss"

interface Props {
    parameterID: string
    label: string
    color: string
    roundFunction?: (value: number) => number
    displayFunction?: (value: number) => string
    style?: React.CSSProperties
}

const Knob: React.FunctionComponent<Props & WithJUCESliderProps> = ({label, parameterID, 
    color, roundFunction, displayFunction, style, value, properties, onChange, 
    reset, dragStart, dragEnd}) => {
    const knobID = useId()
    const labelID = useId()

    const minAngle = -145
    const maxAngle = 145
    const min = properties.start
    const max = properties.end
    const step = (max - min) / (properties.numSteps - 1)
    const angle = functions.remapRange(value, min, max, minAngle, maxAngle)

    if (!roundFunction) roundFunction = (value: number) => value

    if (!displayFunction) displayFunction = (value: number) => `${(value * 100).toFixed(0)}%`

    const keyboardHandler = useKnobKeyboardControls({
        valueRaw: value,
        valueMin: min,
        valueMax: max,
        step: step,
        stepLarger: step * 10,
        onValueRawChange: onChange,
    })

    return (
        <div className="knob-container" style={{...style}}>
            <KnobHeadlessLabel className="knob-label" id={labelID}>
                {label}
            </KnobHeadlessLabel>
            <KnobHeadless
                aria-label={parameterID}
                aria-labelledby={labelID}
                dragSensitivity={0.006}
                onValueRawChange={onChange}
                onDoubleClick={reset}
                valueMin={min}
                valueMax={max}
                valueRaw={value}
                valueRawDisplayFn={displayFunction}
                valueRawRoundFn={roundFunction}
                onDragStart={dragStart}
                onDragEnd={dragEnd}
                axis="y"
                style={{outline: "none"}}
                {...keyboardHandler}>
                <div className="knob" style={{backgroundColor: color}}>
                    <div className="knob-rotator" style={{transform: `rotate(${angle}deg)`}}>
                        <div className="knob-indicator"/>
                    </div>
                </div>
            </KnobHeadless>
            <KnobHeadlessOutput className="knob-value" htmlFor={knobID} style={{color}}>
                {displayFunction(value)}
            </KnobHeadlessOutput>
        </div>
    )
}

export default withJuceSlider(Knob)