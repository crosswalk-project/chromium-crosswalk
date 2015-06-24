// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function injectAndDelete(tabId) {
  // Inject code into each frame. If it's the parent frame, it removes the child
  // frame from the DOM (invalidating it). Thus, the child frame should never
  // have any code injected, because it's been removed. This also caused a
  // crash; crbug.com/500574.
  var injectFrameCode =
      'if (window !== window.top) {' +
      '  chrome.runtime.sendMessage("fail");' +
      '} else {' +
      '  iframe = document.getElementsByTagName("iframe")[0];' +
      '  iframe.parentElement.removeChild(iframe);' +
      '  chrome.runtime.sendMessage("complete");' +
      '}';
  var receivedResponse = false;
  chrome.runtime.onMessage.addListener(function(request, sender, sendResponse) {
    if (request == 'fail') {  // Code shouldn't get injected into the child.
      chrome.test.fail();
    } else {
      chrome.test.assertEq('complete', request);
      receivedResponse = true;
    }
  });
  chrome.tabs.executeScript(
      tabId,
      {code: injectFrameCode, allFrames: true, runAt: 'document_idle'},
      function() {
    chrome.test.assertTrue(receivedResponse);
    chrome.test.succeed();
  });
}

chrome.tabs.onUpdated.addListener(function(tabId, changeInfo, tab) {
  // If we want to inject into subframes using execute script, we need to wait
  // for the tab to finish loading. Otherwise, the frames that would be injected
  // into don't exist.
  if (changeInfo.status != 'complete')
    return;

  chrome.test.runTests([
    function() {
      injectAndDelete(tabId);
    }
  ]);
});

chrome.test.getConfig(function(config) {
  var url = ('http://a.com:PORT/extensions/api_test/executescript/' +
             'removed_frames/outer.html').replace(/PORT/,
                                                     config.testServer.port);
  chrome.tabs.create({url: url});
});
