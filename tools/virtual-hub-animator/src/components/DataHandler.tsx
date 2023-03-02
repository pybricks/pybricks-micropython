// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors
// Inspired by https://github.com/adrianhuber17/webSocket-App

import React, {
    ChangeEvent,
    Dispatch,
    SetStateAction,
    useEffect,
    useState,
} from 'react';

import { Socket } from 'socket.io-client';

interface DataHandlerProps {
    socket: Socket;
    setAngles: Dispatch<SetStateAction<number[]>>;
}

export default function DataHandler(props: DataHandlerProps) {
    const [message, setMessage] = useState('');

    const eventData = 'hubEventData';
    const stateData = 'hubStateData';

    const handleText = (e: ChangeEvent<HTMLInputElement>) => {
        const inputMessage = e.currentTarget.value;
        setMessage(inputMessage);
    };

    const handleSubmit = () => {
        if (!message) {
            return;
        }
        props.socket.emit(eventData, message);
        setMessage('');
    };

    useEffect(() => {
        props.socket.on(stateData, (data) => {
            props.setAngles(data.data);
        });
        return () => {
            props.socket.off(stateData, () => {
                console.log('Closed');
            });
        };
    }, [props]);

    return (
        <div>
            <input type="text" value={message} onChange={handleText} />
            <button onClick={handleSubmit}>Send data to hub</button>
        </div>
    );
}
