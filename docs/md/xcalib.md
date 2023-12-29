# xcalib v0.11.0 {#xcalib}
<a name="toc"></a>
[NAME](#name) [SYNOPSIS](#synopsis) [DESCRIPTION](#description) [OPTIONS](#options) [ENVIRONMENT VARIABLES](#environmentvariables) [EXAMPLES](#examples) [SEE ALSO](#seealso) [AUTHOR](#author) 

<strong>"xcalib"</strong> *1* <em>"December 14, 2023"</em> "User Commands"

<h2>NAME <a href="#toc" name="name">&uarr;</a></h2>

xcalib v0.11.0 - Monitor Calibration Loader

<h2>SYNOPSIS <a href="#toc" name="synopsis">&uarr;</a></h2>

<strong>xcalib</strong> ICC_FILE_NAME
<br />
<strong>xcalib</strong> <a href="#clear"><strong>-c</strong></a> <strong>-d</strong>=<em>STRING</em> <strong>-s</strong>=<em>NUMBER</em> [<strong>-o</strong>=<em>NUMBER</em>] [<strong>-v</strong>]
<br />
<strong>xcalib</strong> <a href="#invert"><strong>-i</strong></a> <strong>-d</strong>=<em>STRING</em> <strong>-s</strong>=<em>NUMBER</em> [<strong>-o</strong>=<em>NUMBER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_FILE_NAME | <strong>-a</strong>
<br />
<strong>xcalib</strong> <a href="#gamma"><strong>-g</strong>=<em>NUMBER</em></a> <strong>-b</strong>=<em>NUMBER</em> <strong>-k</strong>=<em>NUMBER</em> <strong>-d</strong>=<em>STRING</em> <strong>-s</strong>=<em>NUMBER</em> [<strong>-o</strong>=<em>NUMBER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_FILE_NAME | <strong>-a</strong>
<br />
<strong>xcalib</strong> <a href="#red-gamma"><strong>-R</strong>=<em>NUMBER</em></a> <strong>-G</strong>=<em>NUMBER</em> <strong>-B</strong>=<em>NUMBER</em> <strong>-d</strong>=<em>STRING</em> <strong>-s</strong>=<em>NUMBER</em> [<strong>-S</strong>=<em>NUMBER</em>] [<strong>-T</strong>=<em>NUMBER</em>] [<strong>-H</strong>=<em>NUMBER</em>] [<strong>-I</strong>=<em>NUMBER</em>] [<strong>-C</strong>=<em>NUMBER</em>] [<strong>-D</strong>=<em>NUMBER</em>] [<strong>-o</strong>=<em>NUMBER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_FILE_NAME | <strong>-a</strong>
<br />
<strong>xcalib</strong> <a href="#printramps"><strong>-p</strong><em>[=FORMAT]</em></a> <strong>-d</strong>=<em>STRING</em> <strong>-s</strong>=<em>NUMBER</em> [<strong>-o</strong>=<em>NUMBER</em>] [<strong>-v</strong>]
<br />
<strong>xcalib</strong> <a href="#help"><strong>-h</strong><em>[=synopsis|...]</em></a> <strong>-V</strong> <strong>--render</strong>=<em>STRING</em> [<strong>-v</strong>]

<h2>DESCRIPTION <a href="#toc" name="description">&uarr;</a></h2>

The tool loads ’vcgt’‐tag of ICC profiles to the server using the XRandR/XVidMode/GDI Extension in order to load calibrate curves to your graphics card.

<h2>OPTIONS <a href="#toc" name="options">&uarr;</a></h2>

<h3>Set basic parameters</h3>


<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-d</strong>|<strong>--display</strong>=<em>STRING</em></td> <td>host:dpy </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-s</strong>|<strong>--screen</strong>=<em>NUMBER</em></td> <td>Screen Number </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-o</strong>|<strong>--output</strong>=<em>NUMBER</em></td> <td>Output Number<br />It appears in the order as listed in xrandr tool. </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-a</strong>|<strong>--alter</strong></td> <td>Alter Table<br />Works according to parameters without ICC Profile.</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-n</strong>|<strong>--noaction</strong></td> <td>Do not alter video-LUTs.<br />Work's best in conjunction with -v!</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-p</strong>|<strong>--printramps</strong><em>[=FORMAT]</em></td> <td>Print Values on stdout.
  <table>
   <tr><td style='padding-left:0.5em'><strong>-p</strong> TEXT</td><td># TEXT</td></tr>
   <tr><td style='padding-left:0.5em'><strong>-p</strong> SVG</td><td># SVG</td></tr>
  </table>
  </td>
 </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-l</strong>|<strong>--loss</strong></td> <td>Print error introduced by applying ramps to stdout.</td> </tr>
</table>

<h3>Assign</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> ICC_FILE_NAME

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><em>ICC_FILE_NAME</em></td> <td>File Name of a ICC Profile </tr>
</table>

<h3 id="clear">Clear</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-c</strong> <strong>-d</strong>=<em>STRING</em> <strong>-s</strong>=<em>NUMBER</em> [<strong>-o</strong>=<em>NUMBER</em>] [<strong>-v</strong>]

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-c</strong>|<strong>--clear</strong></td> <td>Clear Gamma LUT<br />Reset the Video Card Gamma Table (VCGT) to linear values.</td> </tr>
</table>

<h3 id="invert">Invert</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-i</strong> <strong>-d</strong>=<em>STRING</em> <strong>-s</strong>=<em>NUMBER</em> [<strong>-o</strong>=<em>NUMBER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_FILE_NAME | <strong>-a</strong>

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-i</strong>|<strong>--invert</strong></td> <td>Invert the LUT</td> </tr>
</table>

<h3 id="gamma">Overall Appearance</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-g</strong>=<em>NUMBER</em> <strong>-b</strong>=<em>NUMBER</em> <strong>-k</strong>=<em>NUMBER</em> <strong>-d</strong>=<em>STRING</em> <strong>-s</strong>=<em>NUMBER</em> [<strong>-o</strong>=<em>NUMBER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_FILE_NAME | <strong>-a</strong>

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-g</strong>|<strong>--gamma</strong>=<em>NUMBER</em></td> <td>Specify Gamma: Global gamma correction value (use 2.2 for WinXP Color Control-like behaviour) (NUMBER:1 [≥0.1 ≤5 Δ0.1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-b</strong>|<strong>--brightness</strong>=<em>NUMBER</em></td> <td>Specify Lightness Percentage (NUMBER:0 [≥0 ≤99 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-k</strong>|<strong>--contrast</strong>=<em>NUMBER</em></td> <td>Specify Contrast Percentage: Set maximum value relative to brightness. (NUMBER:100 [≥1 ≤100 Δ1])</td> </tr>
</table>

<h3 id="red-gamma">Per Channel Appearance</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-R</strong>=<em>NUMBER</em> <strong>-G</strong>=<em>NUMBER</em> <strong>-B</strong>=<em>NUMBER</em> <strong>-d</strong>=<em>STRING</em> <strong>-s</strong>=<em>NUMBER</em> [<strong>-S</strong>=<em>NUMBER</em>] [<strong>-T</strong>=<em>NUMBER</em>] [<strong>-H</strong>=<em>NUMBER</em>] [<strong>-I</strong>=<em>NUMBER</em>] [<strong>-C</strong>=<em>NUMBER</em>] [<strong>-D</strong>=<em>NUMBER</em>] [<strong>-o</strong>=<em>NUMBER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_FILE_NAME | <strong>-a</strong>

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-R</strong>|<strong>--red-gamma</strong>=<em>NUMBER</em></td> <td>Specify Red Gamma  (NUMBER:1 [≥0.1 ≤5 Δ0.1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-S</strong>|<strong>--red-brightness</strong>=<em>NUMBER</em></td> <td>Specify Red Brightness Percentage (NUMBER:0 [≥0 ≤99 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-T</strong>|<strong>--red-contrast</strong>=<em>NUMBER</em></td> <td>Specify Red Contrast Percentage: Set maximum value relative to brightness. (NUMBER:100 [≥1 ≤100 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-G</strong>|<strong>--green-gamma</strong>=<em>NUMBER</em></td> <td>Specify Green Gamma  (NUMBER:1 [≥0.1 ≤5 Δ0.1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-H</strong>|<strong>--green-brightness</strong>=<em>NUMBER</em></td> <td>Specify Green Brightness Percentage (NUMBER:0 [≥0 ≤99 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-I</strong>|<strong>--green-contrast</strong>=<em>NUMBER</em></td> <td>Specify Green Contrast Percentage: Set maximum value relative to brightness. (NUMBER:100 [≥1 ≤100 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-B</strong>|<strong>--blue-gamma</strong>=<em>NUMBER</em></td> <td>Specify Blue Gamma  (NUMBER:1 [≥0.1 ≤5 Δ0.1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-C</strong>|<strong>--blue-brightness</strong>=<em>NUMBER</em></td> <td>Specify Blue Brightness Percentage (NUMBER:0 [≥0 ≤99 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-D</strong>|<strong>--blue-contrast</strong>=<em>NUMBER</em></td> <td>Specify Blue Contrast Percentage: Set maximum value relative to brightness. (NUMBER:100 [≥1 ≤100 Δ1])</td> </tr>
</table>

<h3 id="printramps">Show</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-p</strong><em>[=FORMAT]</em> <strong>-d</strong>=<em>STRING</em> <strong>-s</strong>=<em>NUMBER</em> [<strong>-o</strong>=<em>NUMBER</em>] [<strong>-v</strong>]

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-p</strong>|<strong>--printramps</strong><em>[=FORMAT]</em></td> <td>Print Values on stdout.
  <table>
   <tr><td style='padding-left:0.5em'><strong>-p</strong> TEXT</td><td># TEXT</td></tr>
   <tr><td style='padding-left:0.5em'><strong>-p</strong> SVG</td><td># SVG</td></tr>
  </table>
  </td>
 </tr>
</table>

<h3 id="help">General options</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-h</strong><em>[=synopsis|...]</em> <strong>-V</strong> <strong>--render</strong>=<em>STRING</em> [<strong>-v</strong>]

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-h</strong>|<strong>--help</strong><em>[=synopsis|...]</em></td> <td>Print help text<br />Show usage information and hints for the tool.
  <table>
   <tr><td style='padding-left:0.5em'><strong>-h</strong> -</td><td># Full Help : Print help for all groups</td></tr>
   <tr><td style='padding-left:0.5em'><strong>-h</strong> synopsis</td><td># Synopsis : List groups - Show all groups including syntax</td></tr>
  </table>
  </td>
 </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>--render</strong>=<em>STRING</em></td> <td>  </td>
 </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-V</strong>|<strong>--version</strong></td> <td>Version</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-v</strong>|<strong>--verbose</strong></td> <td>Verbose</td> </tr>
</table>


<h2>ENVIRONMENT VARIABLES <a href="#toc" name="environmentvariables">&uarr;</a></h2>

#### DISPLAY
&nbsp;&nbsp;Under X11 systems this variable will hold the display name as used for the -d and -s option.

<h2>EXAMPLES <a href="#toc" name="examples">&uarr;</a></h2>

#### Assign the VCGT curves of a ICC profile to a screen
&nbsp;&nbsp;xcalib ‐d :0 ‐s 0 ‐v profile_with_vcgt_tag.icc
#### Reset a screens hardware LUT in order to do a calibration
&nbsp;&nbsp;xcalib ‐d :0 ‐s 0 ‐c

<h2>SEE ALSO <a href="#toc" name="seealso">&uarr;</a></h2>

&nbsp;&nbsp;[oyjl-args](oyjlargs.html)<a href="oyjlargs.md">(1)</a>


<h2>AUTHOR <a href="#toc" name="author">&uarr;</a></h2>

Stefan Dohla <stefan AT doehla DOT de> http://www.etg.e‐technik.uni‐erlangen.de/web/doe/xcalib/

