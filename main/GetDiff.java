import com.github.gumtreediff.actions.EditScript;
import com.github.gumtreediff.actions.EditScriptGenerator;
import com.github.gumtreediff.actions.SimplifiedChawatheScriptGenerator;
import com.github.gumtreediff.client.Run;
import com.github.gumtreediff.gen.TreeGenerators;
import com.github.gumtreediff.matchers.MappingStore;
import com.github.gumtreediff.matchers.Matcher;
import com.github.gumtreediff.matchers.Matchers;
import com.github.gumtreediff.tree.Tree;
import com.github.gumtreediff.tree.Type;
import com.github.gumtreediff.actions.model.Action;
import com.github.gumtreediff.actions.model.Update;
import com.github.gumtreediff.actions.model.Addition;
import com.github.gumtreediff.actions.model.TreeAddition;
import com.github.gumtreediff.gen.jdt.JdtTreeGenerator;
import com.github.gumtreediff.io.LineReader;
import org.apache.commons.collections.MultiMap;
import org.apache.commons.collections.MultiHashMap;
import java.util.*;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.nio.charset.*;
import java.nio.file.Files;
import java.nio.file.Paths;

@SuppressWarnings("deprecation")
public class GetDiff {
	
	// (start, end) -> slicing criteria
	static String oFileName, pFileName;
	static List<List<Integer>> add=new ArrayList<List<Integer>>(); //lines in pat
	static List<List<Integer>> del=new ArrayList<List<Integer>>(); //lines in ori
	static List<List<Integer>> upt=new ArrayList<List<Integer>>(); //[lines in ori, lines in pat]
	static List<List<Integer>> mov=new ArrayList<List<Integer>>(); //[lines in ori. lines in pat]
	static LineReader olr, plr;

	//get the difference of Tree a and Tree b
	public static List<Action> getActions(Tree a, Tree b) {
		Matcher defaultMatcher = Matchers.getInstance().getMatcher();
		MappingStore mappings = defaultMatcher.match(a, b);
		EditScriptGenerator editScriptGenerator = new SimplifiedChawatheScriptGenerator();
		EditScript actions = editScriptGenerator.computeActions(mappings);
		//System.out.println(actions.asList());
		List<Action> diff = actions.asList();
		//System.out.println(diff.get(1).getName());
		return diff;
	}

	public static List<Integer> getLine(char type, Tree node) throws IOException {
		List<Integer> loc = new ArrayList<Integer>();
		LineReader lr = olr;
		//Tree Tree = new JdtTreeGenerator().generateFrom().reader(lr).getRoot();
		int startPos = node.getPos();
		int len = node.getLength();
		int endPos = startPos+len;
		if (type=='p') {
			lr=plr;
			//System.out.println(node);
		}
		int startLine = lr.positionFor(startPos)[0];
		int endLine = lr.positionFor(endPos)[0];
		loc.add(startLine);
		loc.add(endLine);
		return loc;
	}

	public static void category(String type, Tree node) throws IOException {
		if(type.equals("d")) {
			List<Integer> loc=getLine('o', node);
			List<Integer> tmp=new ArrayList<Integer>();
			for(int i=loc.get(0); i<=loc.get(1); i++) {
				tmp.add(i);
			}
			del.add(tmp);
		}
		else if(type.equals("a")) {
			List<Integer> loc=getLine('p', node);
			List<Integer> tmp=new ArrayList<Integer>();
			for(int i=loc.get(0); i<=loc.get(1); i++) {
				//System.out.println(i);
				tmp.add(i);
			}
			add.add(tmp);
		}
		else if(type.equals("ou")) {
			List<Integer> loc=getLine('o', node);
			List<Integer> tmp=new ArrayList<Integer>();
			tmp.add(loc.get(0));
			if(upt.isEmpty()) {
				upt.add(tmp);
			}
			else {
				upt.get(0).addAll(tmp);
			}
		}
		else if(type.equals("pu")) {
			List<Integer> loc=getLine('p', node);
			List<Integer> tmp=new ArrayList<Integer>();
			tmp.add(loc.get(0));
			if(upt.size()==1) {
				upt.add(tmp);
			}
			else {
				upt.get(1).addAll(tmp);
			}
		}
		else if(type.equals("om")) {
			List<Integer> loc=getLine('o', node);
			List<Integer> tmp=new ArrayList<Integer>();
			for(int i=loc.get(0); i<=loc.get(1); i++) {
				tmp.add(i);
			}
			if(mov.isEmpty()) {
				mov.add(tmp);
			}
			else {
				mov.get(0).addAll(tmp);
			}
		}
		else if(type.equals("pm")) {
			List<Integer> loc=getLine('p', node);
			List<Integer> tmp=new ArrayList<Integer>();
			for(int i=loc.get(0); i<=loc.get(1); i++) {
				tmp.add(i);
			}
			if(mov.size()==1) {
				mov.add(tmp);
			}
			else {
				mov.get(1).addAll(tmp);
			}
		}
	}

	//given actionList, find the lines changed in each action.
	public static void parse(List<Action> actionList, List<Action> actionList1) throws IOException {
		for (int i=0; i<actionList.size(); i++) {
			Action act=actionList.get(i);
			String actionType=act.getName();
			Tree node=act.getNode();

			if(node.toTreeString().contains("comment: ")) {
				continue;
			}

			/*insert-node, insert-tree, delete-node, delete-tree, update-node, move-tree.
			*/
			if (actionType.equals("delete-tree")) { // delete
				category("d", node);
			}
			else if (actionType.equals("insert-tree")) { // insert
				category("a", node);
			}
			else if (actionType.equals("update-node")){ // update
				category("ou", node);
			}
			else if (actionType.equals("move-tree")) { // move
				category("om", node);
			}
		}

		for (int i=0; i<actionList1.size(); i++) {
			Action act=actionList1.get(i);
			String actionType=act.getName();
			Tree node=act.getNode();

			if(node.toTreeString().contains("comment: ")) {
				continue;
			}

			if (actionType.equals("update-node")){ // update
				category("pu", node);
			}
			else if (actionType.equals("move-tree")) { // move
				category("pm", node);
			}
		}
	}

	public static void print(List<List<Integer>> list) {
		System.out.println("###");
		for (int i=0; i<list.size(); i++) {
			for (int j=0; j<list.get(i).size(); j++) {
				System.out.print(list.get(i).get(j)+" ");
			}
			System.out.println("");
		}
	}

	public static void main(String args[]) throws IOException {

		oFileName=args[0];
		pFileName=args[1];
		olr = new LineReader(new FileReader(oFileName));
		Tree oTree = new JdtTreeGenerator().generateFrom().reader(olr).getRoot();
		plr = new LineReader(new FileReader(pFileName));
		Tree pTree = new JdtTreeGenerator().generateFrom().reader(plr).getRoot();

		Run.initGenerators();
		Tree ori = TreeGenerators.getInstance().getTree(oFileName).getRoot();
		Tree pat = TreeGenerators.getInstance().getTree(pFileName).getRoot();
		List<Action> diff=getActions(ori, pat);
		//System.out.println("123");
		//Run.initGenerators();
		Tree ori1 = TreeGenerators.getInstance().getTree(oFileName).getRoot();
		Tree pat1 = TreeGenerators.getInstance().getTree(pFileName).getRoot();
		List<Action> diff1=getActions(pat1, ori1);

		parse(diff, diff1);

		
		//if(!suc) {
			//consider the Fa as the only changed function.
		//}

		print(add);
		print(del);
		print(upt);
		print(mov);

		//get the positions of added statements
		
		//merge the positions of code changes if
		//the positions overlap
		
	}
}

