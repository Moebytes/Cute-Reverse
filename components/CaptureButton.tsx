import React, {useState, useEffect} from "react"
import * as JUCE from "juce-framework-frontend-mirror"
import RecordButton from "../assets/record.svg"
import "./styles/capturebutton.scss"

const startRecording = JUCE.getNativeFunction("startRecording")
const stopRecording = JUCE.getNativeFunction("stopRecording")

const CaptureButton: React.FunctionComponent = () => {
    const [capturing, setCapturing] = useState(false)

    useEffect(() => {
        const eventID = window.__JUCE__.backend.addEventListener("recordingState", updateRecordingState)

        return () => {
            window.__JUCE__.backend.removeEventListener(eventID)
        }
    }, [])

    const updateRecordingState = (state: boolean) => {
        setCapturing(state)
    }
 
    const captureAudio = () => {
        if (capturing) {
            setCapturing(false)
            stopRecording()
        } else {
            setCapturing(true)
            startRecording()
        }
    }

    return (
        <div className={`capture-button ${capturing && "capture-active"}`} onClick={captureAudio}>
            <span className={`capture-button-text ${capturing && "capture-active"}`}>
                {capturing ? "Capturing" : "Capture"}
            </span>
            <RecordButton className={`capture-button-icon ${capturing && "capture-active"}`}/>
        </div>
    )
}

export default CaptureButton