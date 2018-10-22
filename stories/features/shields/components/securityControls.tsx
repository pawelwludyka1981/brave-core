/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Stat,
  GridLabel,
  Toggle,
  EmptyButton,
  ShowMoreIcon,
  ResourcesLabel
} from '../../../../src/features/shields'

// Component groups
import BlockedResources from './blockedReources/blockedResources'
import StaticList from './blockedReources/staticList'

// Fake data
import locale from '../fakeLocale'
import data from '../fakeData'

interface Props {
  enabled: boolean
  favicon: string
  sitename: string
}

interface State {
  openPishingMalwareBlockedList: boolean
  openConnectionsEncryptedList: boolean
}

export default class SecurityControls extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      openPishingMalwareBlockedList: false,
      openConnectionsEncryptedList: false
    }
  }

  get pishingMalwareBlockedList () {
    const { favicon, sitename } = this.props
    return (
      <BlockedResources
        favicon={favicon}
        sitename={sitename}
        title={locale.blockPishing}
        onToggle={this.onTogglePishingMalwareBlocked}
        data={data.thirdPartyDeviceRecognitionBlocked}
      >
        <StaticList
          onClickDismiss={this.onTogglePishingMalwareBlocked}
          list={data.blockedFakeResources}
        />
      </BlockedResources>
    )
  }

  get connectionsEncryptedList () {
    const { favicon, sitename } = this.props
    return (
      <BlockedResources
        favicon={favicon}
        sitename={sitename}
        title={locale.connectionsEncrypted}
        onToggle={this.onToggleConnectionsEncrypted}
        data={data.thirdPartyDeviceRecognitionBlocked}
      >
        <StaticList
          onClickDismiss={this.onToggleConnectionsEncrypted}
          list={data.blockedFakeResources}
        />
      </BlockedResources>
    )
  }

  onTogglePishingMalwareBlocked = () => {
    this.setState({ openPishingMalwareBlockedList: !this.state.openPishingMalwareBlockedList })
  }

  onToggleConnectionsEncrypted = () => {
    this.setState({ openConnectionsEncryptedList: !this.state.openConnectionsEncryptedList })
  }

  render () {
    const { enabled } = this.props
    const { openPishingMalwareBlockedList, openConnectionsEncryptedList } = this.state

    if (!enabled) {
      return null
    }

    return (
      <>
        {/* pishing toggle */}
        <GridLabel>
          <EmptyButton onClick={this.onTogglePishingMalwareBlocked}><ShowMoreIcon /></EmptyButton>
          <Stat>{data.pishingMalwareBlocked}</Stat>
          <ResourcesLabel>{locale.blockPishing}</ResourcesLabel>
          <Toggle checked={true} />
        </GridLabel>
        {openPishingMalwareBlockedList ? this.pishingMalwareBlockedList : null}
        {/* connections encrypted toggle */}
        <GridLabel>
          <EmptyButton onClick={this.onToggleConnectionsEncrypted}><ShowMoreIcon /></EmptyButton>
          <Stat>{data.connectionsEncrypted}</Stat>
          <ResourcesLabel>{locale.connectionsEncrypted}</ResourcesLabel>
          <Toggle checked={true} />
        </GridLabel>
        {openConnectionsEncryptedList ? this.connectionsEncryptedList : null}
      </>
    )
  }
}
