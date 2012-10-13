<?php
   global $base_url;

   $codepsu_path = drupal_get_path('module', 'codepsu');

   // Deal with it
   drupal_add_js('http://ajax.googleapis.com/ajax/libs/jquery/1.3/jquery.min.js');
   drupal_add_js($codepsu_path . '/scripts/countdown.js');

   // Determine the time to put on the timer
   // If the competition is running, but there is no time left, set the timer to 0
   if($node->running && time() > $node->start_time + $node->time_limit*60) {
      $time = 0;
   } else if($node->running) {
      $time = ($node->start_time + ($node->time_limit * 60)) - time();
   } else if(!$node->running) {
      $time = $node->time_limit*60;
   }

   $hours   = floor($time/3600);
   $minutes = floor($time/60) - $hours*60;
   $seconds = $time - $hours*3600 - $minutes*60;

   $show_scoreboard = ($node->running && ($hours > 0 || ($hours <= 0 && $minutes >= 45))) || user_access('administer codepsu');

   // Add a leading zero if necessary
   if($hours < 10) {
      $hours = '0' . $hours;
   }
   if($minutes < 10) {
      $minutes = '0' . $minutes;
   }
   if($seconds < 10) {
      $seconds = '0' . $seconds;
   }

   // Determine number of teams and points
   $num_teams = count($node->teams);
   $num_probs = count($node->points);
?>

<style type="text/css">
   /* style for the colons between the time */
   .cntSeparator {
        font-size: 54px;
        margin:    25px 7px;
        color:     #000000;
   }

   #score_table th {
      text-align: center;
   }

   #score_table td {
      border-width:     2px;
      border-style:     solid;
      border-color:     black;
      padding-top:      5px;
      padding-bottom:   5px;
   }

   .prob_cell {
      text-align: center;
   }

   .even_cell {
      background-color: #EEEEEE;
   }

   .odd_cell {
      background-color: #CCCCCC;
   }

   .low_tier {
      background-color: #E9E6FF;
   }

   .high_tier {
      background-color: #816EFF;
   }
</style>

<script>
   $(document).ready(function() {
      $('#counter').countdown({
        image:       "<?php echo '/' . $codepsu_path . '/images/countdown.png' ?>",
        startTime:   "<?php echo $hours ?>:<?php echo $minutes ?>:<?php echo $seconds ?>",
        format:      "hh:mm:ss",
        start:       <?php if($node->running): ?>1<?php else: ?>0<?php endif; ?>
      });
   });

   <?php if($node->running): ?>
      $('#counter').start();
   <?php endif; ?>
</script>

<strong>Time remaining:</strong><br /><br />
<div id="counter" align="center"></div>

<br /><br />

<?php if($show_scoreboard): ?>
   <strong>Current Standings:</strong>
   <br />
   <p>C = Confirmed correct solution<br />
      U = Unconfirmed correct solution
      <br /><br />
      White = Intermediate tier team<br />
      Blue  = Advanced tier team
   </p>

   <table id="score_table">
      <tr>
         <th width="30%">Team</th>
         <?php for($i=0; $i<$num_probs; $i++): ?>
         <?php echo '<th width="' . round(60/$num_probs) . '%">' . ($i+1) . '</th>'; ?>
         <?php endfor; ?>
         <th width="10%">Total</th>
      </tr>

      <?php
         for($i=0; $i<$num_teams; $i++):
      ?>
      <?php
         echo '<tr>';
         // Write the team name
         echo '<td class="' . (($node->tiers[$i] == 1) ? 'low_tier' : 'high_tier') . '">' . ($i+1) . '.) ' . $node->teams[$i] . '</td>';
         
         // Check each line of the score log for the current team
         $probs = array();
         $confirm = array();
         $log = fopen($node->_path . '/score.log', 'r');
         while(($line = fgets($log)) !== FALSE) {
            if(strpos($line, 'team_' . (($i < 10) ? '0' . ($i+1) : ($i+1)) . ':') !== FALSE) {
               $index = strpos($line, ':');
               $prob_num = (int)substr($line, $index+1, 2);
               $probs[] = $prob_num;
               $confirm[$prob_num] = substr($line, $index+3, 1);
            }
         }
         fclose($log);
         
         // For each problem, write an empty cell or the points if the problem was solved
         $total = 0;
         for($j=0; $j<$num_probs; $j++) {
            echo '<td class="prob_cell ' . (($i%2==0) ? 'even_cell' : 'odd_cell') .  '" width="' . round(60/$num_probs) . '%">';
            if(in_array(($j+1), $probs)) {
               echo $node->points[$j] . $confirm[$j+1];
               $total += $node->points[$j];
            }
            echo '</td>';
         }

         // Write the total number of points
         echo '<td class="prob_cell ' . (($i%2==0) ? 'even_cell' : 'odd_cell') . '">' . $total . '</td>';

         echo '</tr>';
      ?>
      <?php
         endfor; 
      ?>
   </table>
<?php endif; ?>

This page does NOT auto-refresh. You must refresh the page to update the scoreboard. The scoreboard will be hidden during the last 45 minutes of the competition.
