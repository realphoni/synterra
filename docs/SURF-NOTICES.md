# Surf notices — Build 2020

Surf 1.2 displays a native notice strip between the address bar and the tabs.
The page cannot change its text or draw inside it. Warnings use amber;
information uses blue. Messages appear directly, without attribution prefixes.
These are contextual messages, not a malware verdict or a site-blocking system.

- **Dismiss** hides that topic in this tab until it closes. Reloading or switching
  tabs does not undo the dismissal. Menu → **Show notices for this tab** restores it.
- **Next** cycles through multiple matches. Warnings appear first.
- **Read more** opens the notice's source in a new tab in the same window,
  retaining its normal/private profile. Source pages are not fetched in advance.
- Menu → **Site and search notices** toggles the strip. Private windows apply this
  choice only to that window. Normal windows remember it for future windows.
- Settings provides separate switches for all notices, search-topic notices and
  playful messages. Page-load recovery messages remain available when notices
  are disabled. Both strips can appear together.

## Matching and privacy

Matching uses a built-in catalog embedded in the browser. Nothing is downloaded
to decide which notice to show. Site matching compares exact domain names and
their subdomains; URL paths, fragments, arbitrary query strings and page titles
are not keyword-scanned. `roblox.com.evil.example` does not match Roblox.

Search-topic matching reads the search term from DuckDuckGo, Google (`.com`,
`.de`, `.co.uk`), Bing, Brave Search and YouTube search result URLs. It supports
normal form encoding, Unicode case folding and whole-word/phrase boundaries.
Unknown search engines, ambiguous repeated query fields and queries longer than
256 characters are ignored. Search notices explicitly say **Search topic** so a
warning about a result is not mistaken for a warning about the search engine.
No user-entered text is inserted into the notice body or used as HTML.

Dismissals, the selected notice and the rare Ace Attorney message are kept per
tab in memory. They are not added to history, sessions or settings. Private
windows use their existing off-the-record profile. Search URLs remain visible
to the search engine as usual; normal Surf history/session preferences still
apply to ordinary browsing.

## Content

The editable source is [`browser/notices.json`](../browser/notices.json). Rebuild
Surf after changing it; installed files cannot inject new messages at runtime.
The catalog contains 21 topics. The draft's unverified personal allegations are
not included. Roblox's notice points to parental controls; Gemini's notice
describes answer accuracy and confidentiality. The optional playful messages
include community descriptions and jokes; they are editorial copy. The strip
uses only the requested information/warning labels, topic and match context.

Primary sources checked on 2026-10-23:

- [Roblox parental controls](https://en.help.roblox.com/hc/en-us/articles/30428310121620-Parental-Controls-Overview)
- [OpenAI's introduction of ChatGPT](https://openai.com/index/chatgpt/)
- [Gemini Apps Privacy Hub](https://support.google.com/gemini/answer/13594961?hl=en)
- [AO3's ratings guide](https://secure.ao3.org/faq/posting-and-editing?language_id=en)
- [Claude by Anthropic](https://www.anthropic.com/claude)

The rare Ace Attorney text is selected once per tab/topic with a 1-in-20 chance,
so progress updates, zoom changes and tab switching do not repeatedly reroll it.
The catalog is an intentionally small collection, not coverage of all websites.

## Verification

`ctest --test-dir target/surf --output-on-failure` checks catalog validity,
domain boundaries/spoofs, form decoding, Unicode, query limits, warning ordering,
preferences, dismissals and rare variants. `synterra-surf --smoke-test` exercises
the real Qt/Chromium widgets with local fixtures: notice appearance, tab/private
isolation, recovery-bar coexistence, cycling, dismissal/restore, preferences and
light/dark/narrow layouts. No live websites are needed for these checks.

Set `SYNTERRA_SURF_NOTICE_SCREENSHOTS` to an output directory during the smoke
test to save the notice previews. Run WebEngine tests as a regular Linux user
under a desktop or Xvfb, keeping the Chromium sandbox enabled.
