/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWidget, StyledWidgetContainer, StyledWidgetMenuContainer, StyledWidgetMenu, StyledWidgetButton } from './styles'
import { IconButton } from '../../default'
import { CaratCircleODownIcon, CloseStrokeIcon } from 'brave-ui/components/icons'

export interface WidgetProps {
  showWidget: boolean
  menuPosition: 'right' | 'left'
  hideWidget: () => void
}

const createWidget = <P extends object>(WrappedComponent: React.ComponentType<P>) =>
  class Widget extends React.Component<P & WidgetProps> {
    render () {
      const { showWidget, menuPosition, hideWidget } = this.props
      return (
        <StyledWidgetContainer
          showWidget={showWidget}
          menuPosition={menuPosition}
        >
          <StyledWidget>
            <WrappedComponent {...this.props as P}/>
          </StyledWidget>
          <StyledWidgetMenuContainer>
            <IconButton><CaratCircleODownIcon/></IconButton>
            <StyledWidgetMenu
              menuPosition={menuPosition}
            >
            <StyledWidgetButton
              onClick={hideWidget}
            >
              <CloseStrokeIcon/>
              <span>Remove</span>
            </StyledWidgetButton>
            </StyledWidgetMenu>
          </StyledWidgetMenuContainer>
        </StyledWidgetContainer>
      )
    }
  }

export default createWidget
