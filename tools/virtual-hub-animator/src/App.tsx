// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

import './App.css';
import React, { useEffect, useState } from 'react';
import { Socket, io } from 'socket.io-client';
import DataHandler from './components/DataHandler';
import HubVisual from './components/Visual';

function App() {
    const [socketInstance, setSocketInstance] = useState<Socket | null>(null);
    const [loading, setLoading] = useState(true);
    const [angles, setAngles] = useState([0, 0, 0, 0, 0, 0]);
    const [connectStatus, setConnectStatus] = useState(false);

    const handleClick = () => {
        if (connectStatus === false) {
            setConnectStatus(true);
        } else {
            setConnectStatus(false);
        }
    };

    useEffect(() => {
        if (connectStatus === true) {
            const socket = io('localhost:5001/', {
                transports: ['websocket'],
            });

            setSocketInstance(socket);

            socket.on('connect', () => {
                console.log('Connected!');
            });

            setLoading(false);

            socket.on('disconnect', (arg) => {
                console.log('Disconnected!');
                console.log(arg);
                setConnectStatus(false);
            });

            return function cleanup() {
                socket.disconnect();
            };
        }
    }, [connectStatus]);

    return (
        <div className="App">
            {!connectStatus ? (
                <button onClick={handleClick}>Connect</button>
            ) : (
                <>
                    <button onClick={handleClick}>Disconnect</button>
                    <HubVisual motorAngles={angles} />
                    {!loading && socketInstance && (
                        <DataHandler socket={socketInstance} setAngles={setAngles} />
                    )}
                </>
            )}
        </div>
    );
}

export default App;
