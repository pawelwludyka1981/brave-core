/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

interface WidgetContainerProps {
  showWidget: boolean
  menuPosition: 'right' | 'left'
}

export const StyledWidgetContainer = styled<WidgetContainerProps, 'div'>('div')`
  display: ${p => p.showWidget ? 'flex' : 'none'};
  align-items: center;
  flex-direction: ${p => p.menuPosition === 'right' ? 'row' : 'row-reverse'}

  // Always show menu on right on smaller screens
  @media screen and (max-width: 1150px) {
    flex-direction: row;
    padding-left: 48px;
  }
`

export const StyledWidget = styled<{}, 'div'>('div')`
  padding: 24px;

  ${StyledWidgetContainer}:hover & {
    border-radius: 16px;
    background: rgba(33, 37, 41, 0.48);
  }
`

export const StyledWidgetMenuContainer = styled<{}, 'div'>('div')`
  visibility: hidden;
  pointer-events: none;
  ${StyledWidgetContainer}:hover & {
    visibility: visible;
    pointer-events: auto;
  }
  position: relative;
`

interface WidgetMenuProps {
  menuPosition: 'right' | 'left'
}

export const StyledWidgetMenu = styled<WidgetMenuProps, 'div'>('div')`
  position absolute;
  width: 166px;
  padding: 8px 0;
  background: white;
  box-shadow: 0px 0px 6px 0px rgba(0, 0, 0, 0.3);
  border-radius: 4px;
  top: 48px;
  ${p => p.menuPosition === 'right'
  ? 'left: 8px'
  : 'right: 8px'}

  @media screen and (max-width: 1150px) {
    left: 8px;
  }
`

interface WidgetButtonProps {
  onClick: () => void
}
export const StyledWidgetButton = styled<WidgetButtonProps, 'button'>('button')`
  display: flex;
  height: 25px;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  &:hover {
    background: rgb(217, 221, 254);
  }
`
