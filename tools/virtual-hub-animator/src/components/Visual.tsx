// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

import React from 'react';

export interface HubState {
    motorAngles: number[];
}

const HubVisual = (state: HubState) => {
    return (
        <div className="container">
            <div id="backimg"></div>
            <img
                className="wheel-left"
                style={{
                    transform: `translate(542px, -344px) rotate( ${-state
                        .motorAngles[0]}deg )`,
                }}
                src="/img/pbio/wheel-left.png"
            />
            <img className="main-model" src="/img/pbio/main-model.png" />
            <img
                className="shaft"
                style={{
                    transform: `translate(-453px, 212px) rotate( ${state.motorAngles[4]}deg )`,
                }}
                src="/img/pbio/shaft.png"
            />
            <img
                className="stall"
                style={{
                    transform: `translate(-455px, -311px) rotate( ${state.motorAngles[2]}deg )`,
                }}
                src="/img/pbio/stall.png"
            />
            <img
                className="gear-drive"
                style={{
                    transform: `translate(243px, 194px) rotate( ${-state
                        .motorAngles[5]}deg )`,
                }}
                src="/img/pbio/gear-drive.png"
            />
            <img
                className="gear-follow"
                style={{
                    transform: `translate(337px, 194px) rotate( ${
                        state.motorAngles[5] / 3
                    }deg )`,
                }}
                src="/img/pbio/gear-follow.png"
            />
            <img
                className="wheel-right"
                style={{
                    transform: `translate(367px, -169px) rotate( ${state.motorAngles[1]}deg )`,
                }}
                src="/img/pbio/wheel-right.png"
            />
        </div>
    );
};

export default HubVisual;
