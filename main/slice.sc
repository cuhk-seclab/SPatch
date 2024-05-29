import overflowdb.Node
import scala.collection.mutable._
import scala.io.Source
import java.io.File
import java.io.FileWriter

val relations: Map[String, Set[Int]] = Map[String, Set[Int]]()

def GetReLines(inputLines: String, methodName: String) = {
   val sourceNodes: Set[Node] = Set()
   val que: Queue[Node] = Queue()
   val fSlices: Set[Node] = Set()
   val results: Set[Int] = Set()

   /* get AST node of each line in an operation*/
   // inputLines: 1,3,7.
   var lines=inputLines.split(",", 0)
   lines.foreach { line =>
      val sl=cpg.method(methodName).ast.lineNumber(line.toInt).l
      sl.foreach { s =>
         sourceNodes.add(s)
         fSlices.add(s)
      }
   }
   //print("sc"+sourceNodes)

   /* perform forward data-flow slicing on each involved node */
   sourceNodes.foreach { source =>
      que.enqueue(source)
   }
   while (!que.isEmpty) {
      var source = que.dequeue
      var forwards=source.in("REF").dedup.l
      forwards.foreach { forward =>
         if (!fSlices.contains(forward)) {
            que.enqueue(forward)
            fSlices.add(forward)
         }
      }
   }

   /* Map lines in an operation to the affected lines */
   fSlices.foreach { f =>
      var ls=f.id
      val r=cpg.method(methodName).ast.filter(_.id==ls).lineNumber.l
      r.foreach { l =>
         results.add(l)
      }
   }
   relations += (inputLines->results)
}

//example: cpg.bin, foo, 1;2;4.
@main def exec(cpgFile: String, methodName: String, dstFile: String, sourceLine: String) = {
   var gSet: Set[Int] = Set()
   var rSet: Set[Int] = Set()
   var pSet: Set[Int] = Set()
   var cSet: Set[Int] = Set()
   var lSet: Set[Int] = Set() 
   var critical: Set[Int] = Set()
   var cMap: Map[List[Int], Set[Int]] = Map[List[Int], Set[Int]]()
   
   importCpg(cpgFile)

   if (sourceLine!="") {
      var source = sourceLine.split("_", 0)
      // each line represent one operation
      for (line <- source) {
         GetReLines(line, methodName)
      }
   }
   
   //collect non-local assignments
   var localVars=cpg.local.l.name.l
   var localVarLines=cpg.local.l.lineNumber.l
   var assignments=cpg.assignment.l
   assignments.foreach{ assignment =>
      var ass=assignment.code.replaceAll("\\s", "")
      if (ass.contains("=")) {
         var del=ass.indexOf("=")
         var delT=0
         while(del>=delT && (ass(delT)>='a'&&ass(delT)<='z' || ass(delT)>='A'&&ass(delT)<='Z' || ass(delT)>='0'&&ass(delT)<='9' || ass(delT)=='_')) {
            delT=delT+1
         }
         var left=ass.substring(0, delT+1)
         //the var is is not defined locally.
         if(!localVars.contains(left)) {
            var ls=assignment.lineNumber.l
            ls.foreach { l =>
               //print(l)
               //print(methodName)
               //print()
               if(!(localVarLines.contains(l))) {
                  if(left.contains("*")) {
                     pSet.add(l)
                  }
                  else {
                     gSet.add(l)
                     critical.add(l)
                  }
               }
            }
         }
      }
   }

   /* collect return values, filter our error handling code later. */
   var returnVars=cpg.ret.l
   returnVars.foreach{ ret =>
      var ls=ret.lineNumber.l
      ls.foreach { l =>
         //print(l)
         rSet.add(l)
         critical.add(l)
      }
   }

   /* collect call values */
   var callVars=cpg.call.l
   callVars.foreach{ call =>
      var ls=call.lineNumber.l
      ls.foreach { l =>
         //print(l)
         critical.add(l)
      }
   }

   /* Backward intra-proceudral control-dependent slicing on return values. */
   returnVars.foreach { ret =>
      var ls=ret.controlledBy.l.lineNumber.l
      ls.foreach { l =>
         //print(l)
         cSet.add(l)
         critical.add(l)
      }
   }

   /* for, while, and do */
   var forStmts=cpg.forBlock.ast.l
   forStmts.foreach { fo =>
      var fs=fo.lineNumber.l
      fs.foreach { l =>
         lSet.add(l)
      }
   }
   var whileStmts=cpg.whileBlock.ast.l
   whileStmts.foreach { wo =>
      var ws=wo.lineNumber.l
      ws.foreach { l =>
         lSet.add(l)
      }
   }
   var doStmts=cpg.doBlock.ast.l
   doStmts.foreach { d =>
      var ds=d.lineNumber.l
      ds.foreach { l =>
         lSet.add(l)
      }
   }


   val file = new File(dstFile)
   val fw = new FileWriter(file, true)
   fw.write(pSet+"\n")
   fw.write(critical+"\n")
   fw.write(lSet+"\n")
   fw.write(rSet+"\n")
   var isE=relations.isEmpty
   if(isE==true) {
      fw.write("")
   }
   else {
      relations.foreach { case (k, v) =>
         fw.write(k+" "+v.toString)
         //print(v)      
      }
   }
   fw.write("\n")
   fw.close()
}
