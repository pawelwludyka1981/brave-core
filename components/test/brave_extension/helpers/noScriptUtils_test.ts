/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as noScriptUtils from '../../../brave_extension/extension/brave_extension/helpers/noScriptUtils'
import { NoScriptInfo } from '../../../brave_extension/extension/brave_extension/types/other/noScriptInfo'
import { State } from '../../../brave_extension/extension/brave_extension/types/state/shieldsPannelState'
import * as deepFreeze from 'deep-freeze-node'

const url1: string = 'http://aaaa.com/script1.js'
const url2: string = 'http://aaaa.com/script2.js'
const url3: string = 'http://bbbb.com/script1.js'
const url4: string = 'http://cccc.com/script1.js'

const noScriptInfo: NoScriptInfo = {
  [url1]: { actuallyBlocked: false, willBlock: false, userInteracted: false },
  [url2]: { actuallyBlocked: false, willBlock: false, userInteracted: false },
  [url3]: { actuallyBlocked: false, willBlock: true, userInteracted: false },
  [url4]: { actuallyBlocked: false, willBlock: true, userInteracted: false }
}

const url: string = 'https://brave.com'
const tabId: number = 2
const state: State = deepFreeze({
  tabs: { [tabId]: { origin: url, id: tabId, noScriptInfo } },
  windows: { 1: tabId },
  currentWindowId: 1
})

describe('noScriptUtils test', () => {
  describe('filterResourceByOrigin', () => {
    it('filter same origins based on provided url', () => {
      const assertion = noScriptUtils.filterResourceByOrigin(noScriptInfo, url1)
      expect(assertion).toEqual([
        ['http://aaaa.com/script1.js', { actuallyBlocked: false, willBlock: false, userInteracted: false }],
        ['http://aaaa.com/script2.js', { actuallyBlocked: false, willBlock: false, userInteracted: false }]
      ])
    })
  })
  describe('generateNoScriptInfoDataStructure', () => {
    it('generates an array of arrays with items grouped by origin', () => {
      const assertion = noScriptUtils.generateNoScriptInfoDataStructure(noScriptInfo)
      expect(assertion).toEqual([
        [
          'http://aaaa.com',
          [
            ['http://aaaa.com/script1.js', { actuallyBlocked: false, willBlock: false, userInteracted: false }],
            ['http://aaaa.com/script2.js', { actuallyBlocked: false, willBlock: false, userInteracted: false }]
          ]
        ],
        [
          'http://bbbb.com',
          [
            ['http://bbbb.com/script1.js', { actuallyBlocked: false, willBlock: true, userInteracted: false }]
          ]
        ],
        [
          'http://cccc.com',
          [
            ['http://cccc.com/script1.js', { actuallyBlocked: false, willBlock: true, userInteracted: false }]
          ]
        ]
      ])
    })
  })
  describe('filterNoScriptInfoByBlockedState', () => {
    it('filter scripts blocked based on `willBlock` state when its true', () => {
      const modifiedNoScriptInfo = Object.entries(noScriptInfo)
      const assertion = noScriptUtils.filterNoScriptInfoByBlockedState(modifiedNoScriptInfo, true)
      expect(assertion).toEqual([
        ['http://bbbb.com/script1.js', { actuallyBlocked: false, userInteracted: false, willBlock: true }],
        ['http://cccc.com/script1.js', { actuallyBlocked: false, userInteracted: false, willBlock: true }]
      ])
    })
    it('filter scripts blocked based on `willBlock` state when its false', () => {
      const modifiedNoScriptInfo = Object.entries(noScriptInfo)
      const assertion = noScriptUtils.filterNoScriptInfoByBlockedState(modifiedNoScriptInfo, false)
      expect(assertion).toEqual([
        ['http://aaaa.com/script1.js', { actuallyBlocked: false, userInteracted: false, willBlock: false }],
        ['http://aaaa.com/script2.js', { actuallyBlocked: false, userInteracted: false, willBlock: false }]
      ])
    })
  })
  describe('getNoScriptInfo', () => {
    it('gets noScriptInfo data defined in state', () => {
      const assertion = noScriptUtils.getNoScriptInfo(state)
      expect(assertion).toBe(noScriptInfo)
    })
  })
  describe('setNoScriptInfo', () => {
    it('modifies noScript data', () => {
      const url = 'https://awesome-new-website-for-testing-purposes.net'
      const assertion = noScriptUtils.setNoScriptInfo(state, tabId, url, {
        actuallyBlocked: true,
        willBlock: true,
        userInteracted: true
      })
      expect(assertion).toEqual({ ...state, tabs: {
        2: {
          ...state.tabs[2],
          noScriptInfo: {
            'http://aaaa.com/script1.js': { actuallyBlocked: false, userInteracted: false, willBlock: false },
            'http://aaaa.com/script2.js': { actuallyBlocked: false, userInteracted: false, willBlock: false },
            'http://bbbb.com/script1.js': { actuallyBlocked: false, userInteracted: false, willBlock: true },
            'http://cccc.com/script1.js': { actuallyBlocked: false, userInteracted: false, willBlock: true },
            'https://awesome-new-website-for-testing-purposes.net': { actuallyBlocked: true, userInteracted: true, willBlock: true }
          }
        }}
      })
    })
  })
  describe('checkEveryItemIsBlockedOrAllowed', () => {
    it('returns false if all items are blocked but user did not interact', () => {
      const parsedNoScriptInfo = [
        ['http://aaaa.com/script1.js', { actuallyBlocked: false, userInteracted: false, willBlock: true }],
        ['http://aaaa.com/script2.js', { actuallyBlocked: false, userInteracted: false, willBlock: true }]
      ]
      const assertion = noScriptUtils.checkEveryItemIsBlockedOrAllowed(parsedNoScriptInfo, false)
      expect(assertion).toBe(true)
    })
    it('returns true if all items are blocked and user did interact', () => {
      const parsedNoScriptInfo = [
        ['http://aaaa.com/script1.js', { actuallyBlocked: false, userInteracted: true, willBlock: true }],
        ['http://aaaa.com/script2.js', { actuallyBlocked: false, userInteracted: true, willBlock: true }]
      ]
      const assertion = noScriptUtils.checkEveryItemIsBlockedOrAllowed(parsedNoScriptInfo, true)
      expect(assertion).toBe(true)
    })
    it('returns false if all items are allowed and user did not interact', () => {
      const parsedNoScriptInfo = [
        ['http://aaaa.com/script1.js', { actuallyBlocked: false, userInteracted: false, willBlock: false }],
        ['http://aaaa.com/script2.js', { actuallyBlocked: false, userInteracted: false, willBlock: false }]
      ]
      const assertion = noScriptUtils.checkEveryItemIsBlockedOrAllowed(parsedNoScriptInfo, false)
      expect(assertion).toBe(false)
    })
    it('returns true if all items are allowed and user did interact', () => {
      const parsedNoScriptInfo = [
        ['http://aaaa.com/script1.js', { actuallyBlocked: false, userInteracted: true, willBlock: false }],
        ['http://aaaa.com/script2.js', { actuallyBlocked: false, userInteracted: true, willBlock: false }]
      ]
      const assertion = noScriptUtils.checkEveryItemIsBlockedOrAllowed(parsedNoScriptInfo, false)
      expect(assertion).toBe(true)
    })
  })
  describe('getBlockAllText', () => {
    it('returns `allowed once` if all scripts are blocked and user did interact', () => {
      const blockedAndInteractedNoScriptInfo: NoScriptInfo = {
        'aaaa.com/script1.js': { actuallyBlocked: false, willBlock: true, userInteracted: true },
        'aaaa.com/script2.js': { actuallyBlocked: false, willBlock: true, userInteracted: true }
      }
      const assertion = noScriptUtils.getBlockAllText(blockedAndInteractedNoScriptInfo, true)
      expect(assertion).toEqual('Allowed once')
    })
    it('returns `allow` if all scripts are blocked and user did not interact', () => {
      const blockedAndInteractedNoScriptInfo: NoScriptInfo = {
        'aaaa.com/script1.js': { actuallyBlocked: false, willBlock: true, userInteracted: false },
        'aaaa.com/script2.js': { actuallyBlocked: false, willBlock: true, userInteracted: false }
      }
      const assertion = noScriptUtils.getBlockAllText(blockedAndInteractedNoScriptInfo, true)
      expect(assertion).toEqual('Allow all')
    })
    it('returns `blocked once` if all scripts are allowed and user did interact', () => {
      const blockedAndInteractedNoScriptInfo: NoScriptInfo = {
        'aaaa.com/script1.js': { actuallyBlocked: false, willBlock: false, userInteracted: true },
        'aaaa.com/script2.js': { actuallyBlocked: false, willBlock: false, userInteracted: true }
      }
      const assertion = noScriptUtils.getBlockAllText(blockedAndInteractedNoScriptInfo, false)
      expect(assertion).toEqual('Blocked once')
    })
    it('returns `block` if all scripts are allowed and user did interact', () => {
      const blockedAndInteractedNoScriptInfo: NoScriptInfo = {
        'aaaa.com/script1.js': { actuallyBlocked: false, willBlock: false, userInteracted: false },
        'aaaa.com/script2.js': { actuallyBlocked: false, willBlock: false, userInteracted: false }
      }
      const assertion = noScriptUtils.getBlockAllText(blockedAndInteractedNoScriptInfo, false)
      expect(assertion).toEqual('Block all')
    })

  })
  describe('getBlockScriptText', () => {
    it('returns `allowed once` if user did interact and script is blocked', () => {
      const assertion = noScriptUtils.getBlockScriptText(true, true)
      expect(assertion).toBe('Allowed once')
    })
    it('returns `allow` if user did not interact and script is blocked ', () => {
      const assertion = noScriptUtils.getBlockScriptText(false, true)
      expect(assertion).toBe('Allow')
    })
    it('returns `blocked once` if user did interact and script is allowed', () => {
      const assertion = noScriptUtils.getBlockScriptText(true, false)
      expect(assertion).toBe('Blocked once')
    })
    it('returns `block` if did not interact and script is allowed', () => {
      const assertion = noScriptUtils.getBlockScriptText(false, false)
      expect(assertion).toBe('Block')
    })
  })
})